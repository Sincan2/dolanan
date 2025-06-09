import r2pipe

r2 = r2pipe.open('./vmlinux')
r2.cmd('e entry=0xffffffff81e00080')  # set entrypoint secara manual
r2.cmd('aaa')  # analyze all
gadgets = r2.cmd('"/R pop"')  # cari gadget yang ada "pop"

print(gadgets)
