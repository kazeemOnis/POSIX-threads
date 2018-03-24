# POSIX THREADS

Posix threads with c programming, two examples of the use of multithreading is provided below

## Reading Room

C program that simulates readers and writers. A reader thread must enter the room, execute the line read_documents(i.delay, i.id); when it is safe to do so and then leave the room. A writer thread must do the same with write_documents. The following rules apply:
1. A writer may only be in the room if they are the only person in the room. If any readers or writers are in the room, a writer must wait until the room is empty.
2. As long as no writers are in the room, any number of readers can enter the room.
3. Once someone has entered the room, they stay there for the required time
(read_documents/write_documents causes a time delay).
4. The simulation must not deadlock under any circumstances.
5. If any writers are waiting to enter the room, no more readers may enter the room.
Instead they must wait outside until the current occupants have vacated the room and any waiting writers have had their turn.

## Project Marking Problem

The problem described here is used to simulate students and markers participating in assessing MSc projects. There are S students on a course. Each student does a project, which is assessed by K markers. There is a panel of M markers, each of whom is required to assess up to N projects. Students enter the lab at random intervals, All markers are on duty at the beginning of the session and each remains there until they have attended N demos or the session ends. At the end of the session all students and markers must leave the lab. Moreover, any students and markers who are not actively involved in a demo D minutes before the end must leave at that time. 


## Getting Started

Download the zip folder or clone the git repository unto your local repository

```
git clone https://github.com/kazeemOnis/POSIX-threads.git
```

### Running

To run the programs a make file is made available for both files, demo.c and readingroom.c. For the project marking problem run 

```
make demo
```
This creates a demo executable file then run this example to test the program

```
./demo 6 4 2 3 60 5 
```
For the reading room run 

```
make reading room
```
Ths create a readingroom executable file then run this to test the program

```
./readingroom
```

## Built With

* C
* Posix threads


## Authors

* **Kazeem Onisarotu**




