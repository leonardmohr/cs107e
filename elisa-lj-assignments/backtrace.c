#include "backtrace.h"
#include "printf.h"

int backtrace(frame_t f[], int max_frames)
{
    uintptr_t *cur_fp;
    // assembly command to store current frame pointer
    __asm__("mov %0, fp" : "=r" (cur_fp));
    uintptr_t *saved_fp = (uintptr_t *) *(cur_fp - 3);

    int i = 0;
    while (saved_fp != 0 && i < max_frames) {
        // resume_addr is the saved lr of callee
        f[i].resume_addr = (uintptr_t) *(cur_fp - 1);
        f[i].resume_offset = f[i].resume_addr - (*saved_fp - 12);

        // find where the function name is on the heap
        uintptr_t *name_length = (uintptr_t *) (*saved_fp - 16);
        if (*name_length >> 24 != 0xff) {
            f[i].name = "???";
        } else {
            // mask the starting ff of the length
            unsigned int length = (unsigned int) *name_length & 0xffffff;
            f[i].name = (char *) (name_length - (length / 4));
        }
        
        cur_fp = saved_fp;
        saved_fp = (uintptr_t *) *(cur_fp - 3);
        i++;
    }
    return i;
}

void print_frames (frame_t f[], int n)
{
    for (int i = 0; i < n; i++)
        printf("#%d 0x%x at %s+%d\n", i, f[i].resume_addr, f[i].name, f[i].resume_offset);
}

void print_backtrace (void)
{
    int max = 50;
    frame_t arr[max];

    int n = backtrace(arr, max);
    print_frames(arr+1, n-1);   // print frames starting at this function's caller
}
