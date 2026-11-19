"""Generate one child publisher per hub using ESPHome 2026.11 APIs.

Kept component-local so external_components can load this component on its own.
"""

import esphome.codegen as cg
from esphome.core import CORE
from esphome.coroutine import CoroPriority, coroutine_with_priority


class Publisher:
    """Collect child statements into a static function without runtime listeners."""

    def __init__(
        self, name: str, return_type, parameters, *, define: str | None = None
    ) -> None:
        if str(return_type) != "void":
            raise ValueError("Publisher requires a void return type")
        self.name = name
        self.return_type = return_type
        self.parameters = parameters
        self.define = define

    def param(self, name: str) -> cg.MockObj:
        if name not in (param for _, param in self.parameters):
            raise ValueError(f"Unknown publisher parameter: {name}")
        return cg.MockObj(name)

    def _state(self) -> dict:
        return CORE.data.setdefault(__name__, {}).setdefault(
            self.name, {"hubs": {}, "emitted": False}
        )

    def add(self, hub: cg.MockObj, item: cg.Expression | cg.Statement) -> None:
        state = self._state()
        if state["emitted"]:
            raise ValueError(
                "Publisher statements must be added before final code generation"
            )
        if not state["hubs"]:
            CORE.add_job(self._emit)
        state["hubs"].setdefault(str(hub), (hub, []))[1].append(str(cg.statement(item)))

    @coroutine_with_priority(CoroPriority.FINAL)
    async def _emit(self) -> None:
        state = self._state()
        state["emitted"] = True
        if self.define is not None:
            cg.add_define(self.define)
        for hub, statements in state["hubs"].values():
            name = f"esphome__{hub}__{self.name}_fn"
            function = cg.static_function(
                name, self.return_type, self.parameters, statements
            )
            cg.add(getattr(hub, f"set_{self.name}_publisher")(function))
