cmd_/home/francesco/PARSIR/PTE-entry-switcher/the_pte-entry-switcher.ko := ld -r -m elf_x86_64 -z max-page-size=0x200000 -z noexecstack  -T ./scripts/module-common.lds --build-id  -o /home/francesco/PARSIR/PTE-entry-switcher/the_pte-entry-switcher.ko /home/francesco/PARSIR/PTE-entry-switcher/the_pte-entry-switcher.o /home/francesco/PARSIR/PTE-entry-switcher/the_pte-entry-switcher.mod.o ;  true
