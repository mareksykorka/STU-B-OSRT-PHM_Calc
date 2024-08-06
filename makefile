main: main.c server.c clients.c
	make clean
	make build
	@echo ------------------- Running new build ------------------
	make run

build: clients.c server.c main.c
	@echo ------------------ Building Workspace ------------------
	gcc clients.c server.c main.c -o semestralka.o
	@echo ---------------- Normal Build finished -----------------

run: semestralka.o
	@echo ---------------- Starting semestralka.o ----------------
	@echo ---------------- Marek Sykorka-s program ---------------
	@./semestralka.o

debug: clients.c server.c main.c
	make clean
	@echo ------------------ Building Workspace ------------------
	gcc clients.c server.c main.c -o semestralka.o -D DEBUG_INFO
	@echo ----------------- Debug Build finished -----------------

remove_resources:
	@echo ----------------- All System resources -----------------
	@ipcs
	@echo --------------- Force close all resources --------------
	@ipcrm -a
	@echo ---------------- Resources state check. ----------------
	@ipcs
	@echo ----------- Not closed sockets with port 5025. ---------
	@-ss -a | egrep ":5025"
	@echo --- Trying to force close sockets with port of 5025. ---
	@--fuser -k -n tcp 5025
	@echo ----------- Not closed sockets with port 5025 ----------
	@-ss -a | egrep ":5025"
	@echo --------- Try manually "fuser -k -n tcp (port)" --------

print_resources:
	@echo ----------------- All System resources -----------------
	@ipcs
	@echo ------------------ All System sockets ------------------
	@-ss -a | egrep ":5025"
	@echo -------------------- All System PROC -------------------
	@ps -a

clean:
	@echo ------------------ Cleaning Workspace ------------------
	@rm -f *.o
	@echo ------------------- Workspace cleaned ------------------