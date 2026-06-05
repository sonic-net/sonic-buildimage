# Lua Modifications

There are a few modificaitons made from standard lua 5.4.

Refer to `credo-lua-tweaks.patch` for in detail

## C

- `LUA_PATH`/`LUA_CPATH` replaced with `CREDO_LUA_PATH`/`CREDO_LUA_CPATH` to prevent user install lua from messing with
  crlua
- Modified the c print function to call csdk shell logger
- `LUA_LDIR` on linux from `share/lua/5.4/` to `share/credo/lua/` to prevent user installed lua libraries from being
  loaded
- `LUA_CDIR` on linux from `lib/lua/5.4/` to `lib/credo/lua/` to prevent user installed lua libraries from being loaded
- `LUA_IDSIZE` increased from 60 -> 128 for better debug print outs
- force `mkstemp` over `tmpnam` for windows as it is more secure
- Add in eqvalue.patch to give
  [`__eqval` metamethod](https://github.com/geneas/lua/blob/0a4af895c4241d4d183f61b5d66ac38668ed3b1c/eqval544.p)
- Allow printing to use a callback to enable individual print outs

## Lua

- `startup.lua` number has metatable with `value` operator for use with enums
