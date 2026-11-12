CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined
BFLAGS  = -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror
LDLIBS  = -lm
IMPLS   = array list
TESTS   = $(foreach i,$(IMPLS),test_stack_$(i) test_queue_$(i) test_expr_$(i))
BENCHES = $(foreach i,$(IMPLS),bench_$(i))

all: $(TESTS) $(BENCHES) test_contacts calc

# Chaque test est construit avec CHAQUE implementation (array / list).
test_stack_%: test_stack.c stack_%.c stack.h token.h
	$(CC) $(CFLAGS) -o $@ test_stack.c stack_$*.c

test_queue_%: test_queue.c queue_%.c queue.h token.h
	$(CC) $(CFLAGS) -o $@ test_queue.c queue_$*.c

test_expr_%: test_expr.c expr.c expr.h stack_%.c queue_%.c stack.h queue.h token.h
	$(CC) $(CFLAGS) -o $@ test_expr.c expr.c stack_$*.c queue_$*.c $(LDLIBS)

# Benchmark : sans sanitizers, avec -O2, pour des temps representatifs.
bench_%: bench.c stack_%.c queue_%.c stack.h queue.h token.h
	$(CC) $(BFLAGS) -o $@ bench.c stack_$*.c queue_$*.c

test_contacts: test_contacts.c contacts.c contacts.h
	$(CC) $(CFLAGS) -o $@ test_contacts.c contacts.c

calc: calc.c expr.c stack_array.c queue_array.c
	$(CC) $(BFLAGS) -o $@ calc.c expr.c stack_array.c queue_array.c $(LDLIBS)

test: $(TESTS) test_contacts
	for t in $(TESTS) test_contacts; do ./$$t > /dev/null || { echo "ECHEC : $$t"; exit 1; }; echo "ok  $$t"; done

bench: $(BENCHES)
	for b in $(BENCHES); do ./$$b; echo; done

clean:
	rm -f $(TESTS) $(BENCHES) test_contacts calc

.PHONY: all test bench clean
