
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdatomic.h>

int main(int argc, const char *argv[])
{
    atomic_int a;
    int b;

    // init
    atomic_init(&a, 0);

    // load/store
    atomic_store_explicit(&a, 1, memory_order_relaxed);
    assert(atomic_load_explicit(&a, memory_order_relaxed) == 1);
    atomic_store_explicit(&a, 2, memory_order_seq_cst);
    assert(atomic_load_explicit(&a, memory_order_seq_cst) == 2);

    // exchange
    assert(atomic_exchange(&a, 5) == 2);

    // compare-exchange
    b = 5;
    assert(atomic_compare_exchange_weak(&a, &b, 2));
    b = 2;
    assert(atomic_compare_exchange_strong(&a, &b, 5));

    // read-modify-write
    assert(atomic_fetch_add(&a, 5) == 5);
    assert(atomic_fetch_sub(&a, 10) == 10);
    assert(atomic_fetch_or(&a, 0x55AA) == 0);
    assert(atomic_fetch_and(&a, 0x55AA) == 0x55AA);
    assert(atomic_fetch_xor(&a, 0x55AA) == 0x55AA);
    assert(atomic_load(&a) == 0);

    return 0;
}

