PRJ ?= 0
MSG ?= temporary commit

all:
	git add --all
	git commit -m "prj$(PRJ): $(MSG)"
	git push
