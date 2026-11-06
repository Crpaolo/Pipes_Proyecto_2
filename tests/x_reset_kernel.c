// Native integration harness: run reset twice within the same host process.
// This is the only test compiled directly by GCC rather than starc.
#define main selfie_course_entry
#include "../selfie.c"
#undef main

int main(int argc, char** argv) {
  uint64_t* context;
  uint64_t round;

  init_selfie((uint64_t) argc, (uint64_t*) argv);
  init_library();
  init_system();
  init_target();
  init_kernel();
  init_memory(8);

  round = 0;
  while (round < 2) {
    reset_microkernel();
    if (pipe_table != (uint64_t*) 0) return 1;
    if (next_process_id != 1) return 2;
    if (used_contexts != (uint64_t*) 0) return 3;
    context = create_context(MY_CONTEXT, 0);
    set_pc(context, 0);
    if (get_status(context) != CTX_READY) return 4;
    if (get_next_waiter(context) != (uint64_t*) 0) return 5;
    implement_pipe_create(context);
    if (*(get_regs(context) + REG_A0) != 1) return 6;
    if (get_pipe_n(lookup_pipe(1)) != 0) return 7;
    set_pipe_n(lookup_pipe(1), 7);
    next_process_id = 77;
    set_status(context, CTX_EXITED);
    set_next_waiter(context, context);
    round = round + 1;
  }
  printf("ok reset\n");
  return 0;
}
