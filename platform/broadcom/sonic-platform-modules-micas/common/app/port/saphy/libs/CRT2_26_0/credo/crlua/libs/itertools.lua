--- itertools.lua
--- Copyright (C) 2016 Adrian Perez <aperez@igalia.com>
---
--- Distributed under terms of the MIT license.
---
---- Functional iteration utilities using coroutines.
---
--- Iterators
--- ---------
---
--- An **iterator** is a coroutine which yields values of a sequence. Unless
--- specified otherwise, iterators use a constant amount of memory, and
--- yielding the a value takes a constant *O(1)* amount of time.
---
--- Typically iterator implementations use the following pattern:
---
---     function iter (...)
---       -- Do one-time initialization tasks.
---       local finished_iterating = false
---       -- Return a coroutine.
---       return coroutine.wrap(function ()
---         while not finished_iterating do
---           local value = calculate_next_value()
---           coroutine.yield(value)
---         end
---       end)
---     end
---
--- Consuming an iterator is most conveniently done using a `for`-loop:
---
---     for element in iterable do
---       -- Do something with the element.
---     end
---
---
--- Credits
--- -------
---
--- This module is loosely based on [Python's itertools
--- module](https://docs.python.org/3/library/itertools.html), plus some
--- other of Python's built-ins like [map()](https://docs.python.org/3/library/functions.html?highlight=map#map)
--- and [filter()](https://docs.python.org/3/library/functions.html?highlight=filter#filter).
---
---
local pairs, ipairs, select = pairs, ipairs, select
local t_sort, t_unpack = table.sort, table.unpack
local co_yield, co_wrap = coroutine.yield, coroutine.wrap
local co_resume = coroutine.resume

---@module 'itertools'

local itertools = {}

--- Iterate over the keys of a table.
---
--- Given a `table`, returns an iterator over its keys, as returned by
--- `pairs`.
---
--- @param table table<any, any> dictionary-like table.
--- @treturn coroutine An iterator over the table keys.
function itertools.keys(table)
    return co_wrap(function()
        for k, _ in pairs(table) do
            co_yield(k)
        end
    end)
end

--- Iterate over the values of a table.
---
---Given a `table`, returns an iterator over its values, as returned by
--- `pairs`.
---
--- @param table any[] A dictionary-like table.
--- @treturn coroutine An iterator over the table values.
---
function itertools.values(table)
    return co_wrap(function()
        for _, v in pairs(table) do
            co_yield(v)
        end
    end)
end

--- Iterate over the key and value pairs of a table.
---
--- Given a `table`, returns an iterator over its keys and values, as returned
--- by `pairs`. Each yielded element is a two-element *{ key, value }*
--- array-like table.
---
--- Note that yielded array-like tables are not guaranteed be be unique, and if
--- you need to save a copy of it you must create a new table yourself:
---
---    local tpairs = {}
---    for pair in iterable do
---      table.insert(tpairs, { pair[1], pair[2] })
---    end
---
--- @param table table<any,any> dictionary-like table.
--- @treturn coroutine An iterator over *{ key, value }* pairs.
---
function itertools.items(table)
    return co_wrap(function()
        -- Reuse the same table to avoid table creation (and GC) in the loop.
        local pair = {}
        for k, v in pairs(table) do
            pair[1], pair[2] = k, v
            co_yield(pair)
        end
    end)
end

--- Iterate over each value of an array-like table.
---
--- Given an array-like `table`, returns an iterator over its values, as
--- returned by `ipairs`.
---
--- @param table any[] An array-like table.
--- @treturn coroutine An iterator over the table values.
function itertools.each(table)
    return co_wrap(function()
        for _, v in ipairs(table) do
            co_yield(v)
        end
    end)
end

--- Consume an iterable and collect its elements into an array-like table.
---
--- Note that this function runs in *O(n)* time and memory usage because it
--- needs to store all the elements yielded by the iterable.
---
--- @tparam coroutine iterable A non-infinite iterator.
--- @treturn table Array-like table with the collected elements.
--- @treturn integer Number of elements collected.
function itertools.collect(iterable)
    local t, n = {}, 0
    for element in iterable do
        n = n + 1
        t[n] = element
    end
    return t, n
end

--- Iterate over an infinite sequence of consecutive numbers.
---
--- Returns an iterable which produces an infinite sequence of numbers starting
--- at `n`, adding `step` to it in each iteration. Let `i` be the current
--- iteration, starting with `i = 0`, the sequence generated would be:
---
---    n + step * 0, n + step * 1, n + step * 2, ..., n + step * i
---
--- @tparam[opt] number n First value in the sequence.
--- @tparam[opt] number step Increment added in each iteration.
--- @treturn coroutine An iterator over the sequence of numbers.
function itertools.count(n, step)
    if n == nil then
        n = 1
    end
    if step == nil then
        step = 1
    end
    return co_wrap(function()
        while true do
            co_yield(n)
            n = n + step
        end
    end)
end

--- Iterate over a sequence of elements repeatedly.
---
--- Returns an iterable which produces an infinite sequence of elements: first,
--- the elements from `iterable`, then the sequence is repeated indefinitely.
---
--- Note that this may store in memory up to as much elements as provided by
--- `iterable`.
---
--- @tparam coroutine iterable An iterator.
--- @treturn coroutine An infinite iterator repeating elements from `iterable`.
function itertools.cycle(iterable)
    local saved = {}
    local nitems = 0
    return co_wrap(function()
        for element in iterable do
            co_yield(element)
            nitems = nitems + 1
            saved[nitems] = element
        end
        while nitems > 0 do
            for i = 1, nitems do
                co_yield(saved[i])
            end
        end
    end)
end

--- Iterate over the same value repeatedly.
---
--- Returns an iterator which always produces the same `value`, indefinitely or
--- up to a given number of `times`.
---
--- @param value any The value to produce.
--- @tparam[opt] integer times Number of repetitions.
--- @treturn coroutine An iterator which always produces `value`.
---
function itertools.value(value, times)
    if times then
        return co_wrap(function()
            while times > 0 do
                times = times - 1
                co_yield(value)
            end
        end)
    else
        return co_wrap(function()
            while true do
                co_yield(value)
            end
        end)
    end
end

--- Iterate over selected values of an iterable.
---
--- If `start` is specified, the returned iterator will skip all preceding
--- elements; otherwise `start` defaults to `1`. The elements with indexes
--- between `start` and `stop` (inclusive) will be yielded. If `stop` is
--- not specified, the default is to yield all elements from `iterable`
--- until it is exhausted.
---
--- For example, using only `stop` can be used to limit the amount of elements
--- yielded by an indefinite iterator. A `range()` iterator similar to Python's
--- could be implemented as follows:
---
---    function range (n)
---       return itertools.islice(itertools.count(), nil, n)
---    end
---
--- @tparam coroutine iterable An iterator.
--- @tparam[opt] integer start Index of the first element, by default `1`.
--- @tparam[opt] integer stop Index of the last element, by default undefined.
--- @treturn coroutine An iterator which selects values.
function itertools.islice(iterable, start, stop)
    if start == nil then
        start = 1
    end
    return co_wrap(function()
        if stop ~= nil and stop - start < 1 then
            return
        end

        local current = 0
        for element in iterable do
            current = current + 1
            if stop ~= nil and current > stop then
                return
            end
            if current >= start then
                co_yield(element)
            end
        end
    end)
end

--- Iterate over values while a predicate is true.
---
--- The returned iterator returns successive elements from an `iterable` as
--- long as the `predicate` evaluates to `true` for each element.
---
--- @tparam function predicate Function which checks the predicate.
--- @tparam coroutine iterable An iterator.
--- @treturn coroutine An iterator which yield values while the predicate is true.
function itertools.takewhile(predicate, iterable)
    return co_wrap(function()
        for element in iterable do
            if predicate(element) then
                co_yield(element)
            else
                break
            end
        end
    end)
end

--- Iterate over elements applying a function to them
---
--- @tparam function func function Function applied to each element.
--- @tparam coroutine iterable An iterator.
--- @treturn coroutine An iterator which yields the results of applying the
---   function to the elements of `iterable`.
function itertools.map(func, iterable)
    return co_wrap(function()
        for element in iterable do
            co_yield(func(element))
        end
    end)
end

--- Iterate elements, filtering them according to a predicate.
---
--- Returns an iterator over the elements another `iterable` which yields only
--- the elements for which the `predicate` function return `true`.
---
--- For example, the following returns an indefinite iterator over the even
--- natural numbers:
---
---    function even_naturals ()
---       return itertools.filter(function (x) return x % 2 == 1 end,
---                               itertools.count())
---    end
---
--- @tparam function predicate
--- @tparam coroutine iterable An iterator.
--- @treturn coroutine An iterator over the elements which satisfy the predicate.
function itertools.filter(predicate, iterable)
    return co_wrap(function()
        for element in iterable do
            if predicate(element) then
                co_yield(element)
            end
        end
    end)
end

local function make_comp_func(key)
    if key == nil then
        return nil
    end
    return function(a, b)
        return key(a) < key(b)
    end
end

local _collect = itertools.collect

--- Iterate over the sorted elements from an iterable.
---
--- A custom `key` function can be supplied, and it will be applied to each
--- element being compared to obtain a sorting key, which will be the values
--- used for comparisons when sorting. The `reverse` flag can be set to sort
--- the elements in descending order.
---
--- Note that `iterable` must be consumed before sorting, so the returned
--- iterator runs in *O(n)* memory space. Sorting is done internally using
--- `table.sort`.
---
--- @tparam coroutine iterable An iterator.
--- @tparam[opt] function key Function used to retrieve the sorting key used
---   to compare elements.
--- @tparam[opt] boolean reverse Whether to yield the elements in reverse
---   (descending) order. If not supplied, defaults to `false`.
--- @treturn coroutine An iterator over the sorted elements.
function itertools.sorted(iterable, key, reverse)
    local t, n = _collect(iterable)
    t_sort(t, make_comp_func(key))
    if reverse then
        return co_wrap(function()
            for i = n, 1, -1 do
                co_yield(t[i])
            end
        end)
    else
        return co_wrap(function()
            for i = 1, n do
                co_yield(t[i])
            end
        end)
    end
end

local _value = itertools.value

--- Cartesian product of iterables.
---
--- This is equivalent to nested for-loops, with the leftmost iterators
--- being the outermost for-loops, so the yielded results cycle in a
--- manner similar to an odometer, with the rightmost element changing
--- on every iteration.
---
--- @tparam coroutine iterable An iterator.
--- @tparam[opt] coroutine ... Additional iterators.
--- @treturn coroutine An iterator over tuples of product elements.
function itertools.product(...)
    local niterables = select('#', ...)
    local items = {}
    for i = 1, niterables do
        items[i] = _collect(select(i, ...))
    end
    return co_wrap(function()
        local indices, _ = _collect(_value(1, niterables))
        while true do
            -- Assemble and yield a result using the current indices.
            local result = {}
            for i = 1, niterables do
                result[i] = items[i][indices[i]]
            end
            co_yield(t_unpack(result))

            -- Update the indices, right to left, advancing to the
            -- next column when the previous one has rolled over.
            local last_rolled = nil
            for i = niterables, 1, -1 do
                indices[i] = indices[i] + 1
                if indices[i] > #items[i] then
                    -- Roll over.
                    last_rolled = i
                    indices[i] = 1
                    result[i] = items[i][indices[i]]
                else
                    -- No roll over.
                    result[i] = items[i][indices[i]]
                    break
                end
            end
            -- All the columns rolled over: stop.
            if last_rolled == 1 then
                return
            end
        end
    end)
end

function itertools.product_table(...)
    local niterables = select('#', ...)
    local items = {}
    for i = 1, niterables do
        items[i] = _collect(select(i, ...))
    end
    return co_wrap(function()
        local indices, _ = _collect(_value(1, niterables))
        while true do
            -- Assemble and yield a result using the current indices.
            local result = {}
            for i = 1, niterables do
                result[i] = items[i][indices[i]]
            end
            co_yield(result)

            -- Update the indices, right to left, advancing to the
            -- next column when the previous one has rolled over.
            local last_rolled = nil
            for i = niterables, 1, -1 do
                indices[i] = indices[i] + 1
                if indices[i] > #items[i] then
                    -- Roll over.
                    last_rolled = i
                    indices[i] = 1
                    result[i] = items[i][indices[i]]
                else
                    -- No roll over.
                    result[i] = items[i][indices[i]]
                    break
                end
            end
            -- All the columns rolled over: stop.
            if last_rolled == 1 then
                return
            end
        end
    end)
end

return itertools
