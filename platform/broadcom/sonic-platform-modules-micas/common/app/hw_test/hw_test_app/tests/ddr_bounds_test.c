#include <assert.h>
#include <stdint.h>

#include "../ft_ddr_test.c"

#define GUARD_PATTERN 0xdeadbeefu
#define TEST_PATTERN 0x55555555u

int platform_fac_dbg = 0;

static void check_steps(void)
{
    uint32 memory[17];
    volatile uint32 *current = memory;

    assert(bounded_step(memory, memory, 8) == 0);
    assert(bounded_step(memory, memory + 7, 8) == 7);
    assert(bounded_step(memory, memory + 8, 8) == 8);
    assert(bounded_step(memory, memory + 17, 8) == 8);
    assert(bounded_step(memory, memory + 17, SIZE_MAX) == 17);
    assert(bounded_step(memory, memory + 17, 0) == 0);
    current += bounded_step(current, memory + 17, 8);
    assert(current == memory + 8);
    current += bounded_step(current, memory + 17, 8);
    assert(current == memory + 16);
    current += bounded_step(current, memory + 17, 8);
    assert(current == memory + 17);
    current += bounded_step(current, memory + 17, 8);
    assert(current == memory + 17);
    current -= bounded_step(memory, current, 8);
    assert(current == memory + 9);
    current -= bounded_step(memory, current, 8);
    assert(current == memory + 1);
    current -= bounded_step(memory, current, 8);
    assert(current == memory);
    current -= bounded_step(memory, current, 8);
    assert(current == memory);
}

static void check_bounds(const uint32 *storage, size_t words)
{
    assert(storage[0] == GUARD_PATTERN);
    assert(storage[words + 1] == GUARD_PATTERN);
    assert((uintptr_t)pp >= (uintptr_t)(storage + 1));
    assert((uintptr_t)pp <= (uintptr_t)(storage + words + 1));
}

static void check_uniform(const uint32 *storage, size_t words, uint32 pattern)
{
    size_t i;

    check_bounds(storage, words);
    for (i = 0; i < words; ++i) {
        assert(storage[i + 1] == pattern);
    }
}

static void check_range(size_t words)
{
    uint32 *storage = malloc((words + 2) * sizeof(*storage));
    char desc[128] = {0};
    size_t i;

    assert(storage != NULL);
    storage[0] = GUARD_PATTERN;
    storage[words + 1] = GUARD_PATTERN;
    ft_ddr_test_init(storage + 1, words * sizeof(*storage));

    assert(moving_inversions(desc, 2, TEST_PATTERN, ~TEST_PATTERN) == FT_DDR_SUCCESS);
    check_uniform(storage, words, TEST_PATTERN);
    assert(march_c(desc, 2, TEST_PATTERN, ~TEST_PATTERN) == FT_DDR_SUCCESS);
    check_uniform(storage, words, TEST_PATTERN);
    assert(march_g(desc, 2, TEST_PATTERN, ~TEST_PATTERN) == FT_DDR_SUCCESS);
    check_uniform(storage, words, TEST_PATTERN);
    assert(galloping(desc, TEST_PATTERN, ~TEST_PATTERN) == FT_DDR_SUCCESS);
    check_uniform(storage, words, ~TEST_PATTERN);

    assert(moving_inversions32(desc, 2, 1, 1, 0x80000000u, 0, 0) == FT_DDR_SUCCESS);
    check_bounds(storage, words);
    for (i = 0; i < words; ++i) {
        assert(storage[i + 1] == (1u << (i % 32)));
    }

    free(storage);
    printf("\nDDR boundary range: %zu words passed\n", words);
}

int main(void)
{
    const size_t sizes[] = {
        0, 1, SPINSZ - 1, SPINSZ, SPINSZ + 1, SPINSZ + 2,
        2 * SPINSZ, 2 * SPINSZ + 1, 2 * SPINSZ + 2
    };
    size_t i;

    check_steps();
    for (i = 0; i < sizeof(sizes) / sizeof(sizes[0]); ++i) {
        check_range(sizes[i]);
    }
    return 0;
}
