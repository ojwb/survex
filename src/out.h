#define out_current_action(SZ) printf("\n%s...\n", (SZ))
/* unfortunately we'll have to remember to double bracket ... */
#define out_printf(X) do{printf X;putchar('\n');}while(0)
#define out_puts(SZ) puts((SZ))
#define out_set_percentage(P) printf("%d%%\r", (int)(P))
#define out_set_fnm(SZ) if (!fPercent) NOP; else printf("%s:\n", (SZ))
