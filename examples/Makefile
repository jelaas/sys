all:	stor
stor:	storage/main.js storage/content1
	wc -c $^ > $@
	cat $^ >> $@
clean:
	stor
