# use "make lustre" if you have lustre, "make nolustre" if not

lustre:
	gcc -o shift-bin -g shift-bin.c -lattr -lacl -llustreapi

nolustre:
	gcc -o shift-bin -g shift-bin.c -lattr -lacl -D_NO_LUSTRE
