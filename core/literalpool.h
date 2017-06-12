#ifndef LITERALPOOL_H
#define LITERALPOOL_H

/* Code shared by various translators, this file keeps the
   arch-independant parts. */

struct LiteralRef {
	void *inst;
	uintptr_t value;
};

static constexpr size_t MAX_LITERALS = 1024;
static LiteralRef literals[MAX_LITERALS];
static size_t literals_count = 0;

void literalpool_add(uintptr_t value)
{
	if(literals_count >= MAX_LITERALS)
	{
		literals_count = 0; // Otherwise it won't ever be empty again
		error("Literal pool full, please increase the size");
		return;
	}

	literals[literals_count++] = LiteralRef { .inst = translate_current,
	                                          .value = value };
}

/* Function implemented by the architecture.
   It needs to iterate through all elements in literals until literals_count,
   emit the literal into a suitable location and fixup all instructions that
   reference it. If alignment is not an issue, certain values can be optimized,
   but in such cases the loading instruction and the literal pool need to agree.
   It then needs to reset literals_count to 0. */
void literalpool_fill();

#endif //LITERALPOOL_H
