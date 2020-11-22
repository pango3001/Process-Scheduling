# CS 4760 Operating Systems

Assignment # 4 Due Date: October 30, 2020

# Process Scheduling

# Purpose

The goal of this homework is to learn about process scheduling inside an operating system. You will work on the specified scheduling
algorithm and simulate its performance.

# IMPORTANT NOTE
if program hangs, perform a make clean then rebuild (should not be an issue but just in case)
if problems persist perform ipcrm -add

# Task

In this project, you will simulate the process scheduling part of an operating system. You will implement time-based scheduling,
ignoring almost every other aspect of theOS. Please use message queues for synchronization.

Operating System Simulator

The operating system simulator, orOSS, will be your main program and serve as the master process. You will start the simulator (call
the executableoss) as one main process who will fork multiple children at random times. The randomness will be simulated by a
logical clock that will also be updated byoss.

In the beginning,osswill allocate shared memory for system data structures, including a process table with a process control block for
each user process. The process control block is a fixed size structure and contains information to manage the child process scheduling.
Notice that since it is a simulator, you will not need to allocate space to save the context of child processes. But you must allocate
space for scheduling-related items such as totalCPUtime used, total time in the system, time used during the last burst, your local
simulated pid, and process priority, if any. The process control block resides in shared memory and is accessible to the children. Since
we are limiting ourselves to 20 processes in this class, you should allocate space for up to 18 process control blocks. Also create a bit
vector, local tooss, that will help you keep track of the process control blocks (or processIDs) that are already taken.

osssimulates the passing of time by using a simulated system clock. The clock is stored in two shared integers in memory, one which
stores seconds and the other nanoseconds. so use two unsigned integers for the clock.osswill be creating user processes at random
intervals, say every second on an average. While the simulated clock (the two integers) is viewable by the child processes, it should
only be advanced (changed) byoss.

Note thatosssimulates time passing in the system by adding time to the clock and as it is the only process that would change the
clock, if a user process uses some time,ossshould indicate this by advancing the clock. In the same fashion, ifossdoes something
that should take some time if it was a real operating system, it should increment the clock by a small amount to indicate the time it
spent.

osswill create user processes at random intervals (of simulated time), so you will have two constants; let us call them
maxTimeBetweenNewProcsNSandmaxTimeBetweenNewProcsSecs.osswill launch a new user process based on a ran-
dom time interval from 0 to those constants. Itgeneratesa new process by allocating and initializing the process control block for
the process and then,forks the process. The child process willexeclthe binary. I would suggest setting these constants initially
to spawning a new process about every 1 second, but you can experiment with this later to keep the system busy. New processes that
are created can have one of two scheduling classes, either real-time or a normal user process, and will remain in that class for their
lifetime. There should be constant representing the percentage of time a process is launched as a normal user process or a real-time
one. While this constant is specified by you, it should be heavily weighted to generating mainly user processes.

osswill run concurrently with all the user processes. After it sets up all the data structures, it enters a loop where it generates and
schedules processes. Itgeneratesa new process by allocating and initializing the process control block for the process and then,forks
the process. The child process willexeclthe binary.

osswill be in control of all concurrency. In the beginning, there will be no processes in the system but it will have a time in the future
where it will launch a process. If there are no processes currently ready to run in the system, it should increment the clock until it is the
time where it should launch a process. It should then set up that process, generate a new time where it will launch a process and then
using a message queue, schedule a process to run by sending it a message. It should then wait for a message back from that process


that it has finished its task. If your process table is already full when you go to generate a process, just skip that generation, but do
determine another time in the future to try and generate a new process.

Advance the logical clock by 1.xx seconds in each iteration of the loop where xx is the number of nanoseconds. xx will be a random
number in the interval [0,1000] to simulate some overhead activity for each iteration.

A new process should be generated every 1 second, on an average. So, you should generate a random number between 0 and 2
assigning it to time to create new process. If your clock has passed this time since the creation of last process, generate a new process
(andexeclit).

ossacts as the scheduler and so willschedulea process by sending it a message using a message queue. When initially started, there
will be no processes in the system but it will have a time in the future where it will launch a process. If there are no processes currently
ready to run in the system, it should increment the clock until it is the time where it should launch a process. It should then set up that
process, generate a new time where it will create a new process and then using a message queue, schedule a process to run by sending
it a message. It should then wait for a message back from that process that it has finished its task. If your process table is already full
when you go to generate a process, just skip that generation, but do determine another time in the future to try and generate a new
process.

Scheduling Algorithm

Assuming you have more than one process in your simulated system,osswillselecta process to run andscheduleit for execution. It
will select the process by using a scheduling algorithm with the following features:

Implement a multi-level feedback queue with Linux-like scheduling. That is, you will have two classes of processes: active and
expired. When a process has exhausted its quantum, it will be moved to expired set.osswill always pick up the process with highest
priority that is acive. There are four scheduling queues, each having an associated time quantum. The base time quantum is determined
by you as a constant, let us say something like 10 milliseconds, but certainly could be experimented with. The highest priority queue
has this base time quantum as the amount of time it would schedule a child process if it scheduled it out of that queue. The second
highest priority queue would have half of that, the third highest priority queue would have quarter of the base queue quantum and so
on, as per a normal multi-level feedback queue. If a process finishes using its entire timeslice for the queue, it should be moved to a
queue one lower in priority. If a process comes out of a blocked queue, it should go to the highest priority queue.

Whenosshas to pick a process to schedule, it will look for the highest priority occupied queue and schedule the process at the head
of this queue. The process will bedispatchedby sending the process a message using a message queue indicating how much of a time
slice it has to run. Note that this scheduling itself takes time, so before launching the process theossshould increment the clock for
the amount of work that it did, let us say from 100 to 10000 nanoseconds.

User Processes

All user processes are alike but simulate the system by performing some tasks at random times. The user process will keep checking
in the shared memory location if it has been scheduled and once scheduled, it will start to run. It should generate a random number to
check whether it will use the entire quantum, or only a part of it (a binary random number will be sufficient for this purpose). If it has
to use only a part of the quantum, it will generate a random number in the range [0,quantum] to see how long it runs.

The user processes will wait on receiving a message giving them a time slice and then it will simulate running. They do not do any
actualwork but instead send a message toosssaying how much time they used and if they had to useI/Oor had to terminate.

As a constant in your system, you should have a probability that a process will terminate when scheduled. I would suggest this
probability be fairly small to start. Processes will then, using a random number, use this to determine if they should terminate. Note
that when creating this random number you must be careful that the seed you use is different for all processes, so I suggest seeding
off of some function of a processes pid. If it would terminate, it would of course use some random amount of its timeslice before
terminating. It should indicate toossthat it has decided to terminate and also how much of its timeslice it used, so thatosscan
increment the clock by the appropriate amount.

Once it has decided that it will not terminate, then we have to determine if it will use its entire timeslice or if it will get blocked on an
event. This should be determined by a random number between 0 and 1. If it uses up its timeslice, this information should be conveyed
to master. Otherwise, the process starts to wait for an event that will last forr.sseconds whererandsare random numbers with
range[0,3]and[0,1000]respectively, and 3 indicates that the process gets preempted after usingp%of its assigned quantum, where
pis a random number in the range[1,99]. As this could happen for multiple processes, this will require a blocked queue, checked by
ossevery time it makes a decision on scheduling to see if it should wake up these processes and put them back in the appropriate


queues. Note that the simulated work of moving a process from a blocked queue to a ready queue would take more time than a normal
scheduling decision so it would make sense to increment the system clock to indicate this.

Your simulation should end with a report on average wait time, averageCPUutilization and average time a process waited in a blocked
queue. Also include how long theCPUwas idle with no ready processes.

Make sure that you have signal handing to terminate all processes, if needed. In case of abnormal termination, make sure to remove
shared memory and message queues.

# Invoking the solution

Executeosswith no parameters. You may add some parameters to modify the number of processes or base quantum but if you do so,
document this in yourREADME.

Log Output

Your program should send enough output to a log file such that it is possible for me to determine its operation. For example:

OSS: Generating process with PID 3 and putting it in queue 1 at time 0:
OSS: Dispatching process with PID 2 from queue 1 at time 0:5000805,
OSS: total time this dispatch was 790 nanoseconds
OSS: Receiving that process with PID 2 ran for 400000 nanoseconds
OSS: Putting process with PID 2 into queue 2
OSS: Dispatching process with PID 3 from queue 1 at time 0:5401805,
OSS: total time this dispatch was 1000 nanoseconds
OSS: Receiving that process with PID 3 ran for 270000 nanoseconds,
OSS: not using its entire time quantum
OSS: Putting process with PID 3 into queue 1
OSS: Dispatching process with PID 1 from queue 1 at time 0:5402505,
OSS: total time spent in dispatch was 7000 nanoseconds
etc

I suggest not simply appending to previous logs, but start a new file each time. Also be careful about infinite loops that could generate
excessively long log files. So for example, keep track of total lines in the log file and terminate writing to the log if it exceeds 10000
lines.

Note that the above log was using arbitrary numbers, so your times spent in dispatch could be quite different.

# Suggested implementation steps

I highly suggest you do this project incrementally. I suggest implementation in the following order:

- Start by creating aMakefilethat compiles and builds the two executables:ossanduser_proc.
- Haveosscreate a process control table with one user process (of real-time class) to verify it is working
- Schedule the one user process over and over, logging the data
- Create the round robin queue, add additional user processes, making all user processes alternate in it
- Keep track of and output statistics like throughput, idle time, etc
- Implement an additional user class and the multi-level feedback queue.
- Add the chance for user processes to be blocked on an event, keep track of statistics on this

Do not try to do everything at once and be stuck with no idea what is failing.


Termination Criteria

ossshould stop generating processes if it has already generated 100 processes or if more than 3 real-life seconds have passed. If you
stop adding new processes, the system should eventually empty of processes and then it should terminate. What is important is that
you tune your parameters so that the system has processes in all the queues at some point and that I can see that in the log file. As
discussed previously, ensure that appropriate statistics are displayed.

# Criteria for success

Make sure that you implement the specified algorithm and document it appropriately.You must clean up after yourself. That is, after
the program terminates, whether normally or by force, there should be no shared memory, semaphore, or message queue that is left
allocated to you.

# Grading

1. Overall submission: 30pts. Program compiles and upon reading, seems to solve the assigned problem in the specified manner.
2. README/Makefile: 10pts. Ensure that they are present and work appropriately.
3. Code readability: 10pts. Code should be readable with appropriate comments. Author and date should be identified.
4. Conformance to specifications: 50pts. Algorithm is properly implemented and documented.

Submission

Handin an electronic copy of all the sources,README, Makefile(s), and results. Create your programs in a directory calledusername.
whereusernameis your login name on hoare. Once you are done with everything,remove the executables and object files, and issue
the following commands:

% cd
% chmod 755 ̃
% ̃sanjiv/bin/handin cs4760 4
% chmod 700 ̃

Do not forgetMakefile(with suffix rules), version control, andREADMEfor the assignment.




commits
commit 6b7260d7dbf03d8cdc41df9c79ac7f2d8d1b539d
Author: pango3001 <pango3001@gmail.com>
Date:   Mon Nov 2 19:52:25 2020 -0600

    updates

commit a82790912de0895b48d4f28fb513f0fe7a1b294c
Author: pango3001 <pango3001@gmail.com>
Date:   Mon Nov 2 19:51:29 2020 -0600

    updates

commit 7bd9b1655c0394c403f6d0011c9c75f60763acfa
Author: pango3001 <pango3001@gmail.com>
Date:   Mon Nov 2 19:49:51 2020 -0600

    update

commit 3fdb172834e1fcf5f4d007b6eee8c828e0cec47f
Author: pango3001 <pango3001@gmail.com>
Date:   Mon Nov 2 19:48:07 2020 -0600

    Update user_proc.c

commit bb5f06f7b7b939a7fe0d67def1d28d25eeceda1e
Author: pango3001 <pango3001@gmail.com>
Date:   Mon Nov 2 19:46:10 2020 -0600

    Update oss.c

commit 00e47278313bdeaab41a7ccfbca542fd564f2fdb
Author: pango3001 <pango3001@gmail.com>
Date:   Mon Nov 2 19:43:44 2020 -0600

    Update oss.c

commit e415720cb09c7256f42dffe66e328b99b5b96042
Author: pango3001 <pango3001@gmail.com>
Date:   Mon Nov 2 19:41:55 2020 -0600

    updates

commit 6c4167e817b173f4d4e504b630aa8e62cabf1209
Author: pango3001 <pango3001@gmail.com>
Date:   Mon Nov 2 19:39:17 2020 -0600

    updates

commit 9ab260f58b99a0bef414a4572f8dc46c3b811aa3
Author: pango3001 <pango3001@gmail.com>
Date:   Mon Nov 2 19:37:04 2020 -0600

    updates

commit e70ac34a07761ac8d3247bddd7a53a8d2baa0e6f
Author: pango3001 <pango3001@gmail.com>
Date:   Mon Nov 2 19:33:47 2020 -0600

    Update oss.c

commit 116f0e3176d7dc3c6006172ee545c6fbf3ff62c1
Author: pango3001 <pango3001@gmail.com>
Date:   Mon Nov 2 19:33:16 2020 -0600

    Update user_proc.c

commit 8edcdc7e863ffbf7c8ecaf540f3c6cdef2142961
Author: pango3001 <pango3001@gmail.com>
Date:   Mon Nov 2 19:32:20 2020 -0600

    added ftok for keys

commit 2f46544fb7056930348a10ab0e6cfeca9823f4ab
Author: pango3001 <pango3001@gmail.com>
Date:   Mon Nov 2 19:25:14 2020 -0600

    Update oss.c

commit 7a1cd3fe36c68afb96c7452150aa7a7689f9e1b2
Author: pango3001 <pango3001@gmail.com>
Date:   Mon Nov 2 19:21:52 2020 -0600

    Update oss.c

commit c18bc97b54726616ec95b01f8f21812c56105d4f
Author: pango3001 <pango3001@gmail.com>
Date:   Mon Nov 2 19:16:28 2020 -0600

    Update oss.c

commit d35ecad89d3dd7e28f13efddacf1234061e0ff50
Author: pango3001 <pango3001@gmail.com>
Date:   Mon Nov 2 19:15:03 2020 -0600

    Update oss.c

commit efffbae15484737b0a3e29ab0871be0c7e14aebf
Author: pango3001 <pango3001@gmail.com>
Date:   Mon Nov 2 19:06:41 2020 -0600

    Update oss.c

commit cdc40ee72f521ea8538354d1e961dcb66b47b138
Author: pango3001 <pango3001@gmail.com>
Date:   Mon Nov 2 19:05:00 2020 -0600

    Update oss.c

commit 286f4c85ff2e332a33d0cffd9d450284e3491bf4
Author: pango3001 <pango3001@gmail.com>
Date:   Mon Nov 2 18:59:14 2020 -0600

    updates

commit 1d8f1e9fb06a3dd2869f5a5c285fdff85ffc29a8
Author: pango3001 <pango3001@gmail.com>
Date:   Mon Nov 2 16:17:09 2020 -0600

    Update oss.c

commit 4db6c0fe85d6897222226eeda31f2e3b5170ff92
Merge: c0e7366 176cac9
Author: pango3001 <pango3001@gmail.com>
Date:   Mon Nov 2 16:14:44 2020 -0600

    Merge branch 'main' of https://github.com/pango3001/process_scheduling into main

commit 176cac99e077da134b20fbef0291b3ba169e4ed3
Author: Jesse McCarville <pango3001@gmail.com>
Date:   Mon Nov 2 16:13:42 2020 -0600

    updates

commit c0e7366be035290aa8fa412de804a0147952bb14
Author: pango3001 <pango3001@gmail.com>
Date:   Mon Nov 2 16:12:17 2020 -0600

    Update oss.c

commit 556d2832383fb0b5f16c3d86c68277dbc742ad15
Author: pango3001 <pango3001@gmail.com>
Date:   Mon Nov 2 14:07:11 2020 -0600

    Update oss.c

commit 5c972b1c91bcba3b517d11695f1131522cd81cfe
Author: pango3001 <pango3001@gmail.com>
Date:   Mon Nov 2 14:06:03 2020 -0600

    Update oss.c

commit fbbeb0de4dc17ad708a62e7fa79a295407c33838
Author: pango3001 <pango3001@gmail.com>
Date:   Mon Nov 2 14:01:20 2020 -0600

    Update oss.c

commit aab0f2f00f3c0297086f946c36b03c4e032afb2c
Author: pango3001 <pango3001@gmail.com>
Date:   Mon Nov 2 13:38:03 2020 -0600

    Update oss.c

commit 0edf9ffe31b46f2bfc18ba0f7562a43736ef5cea
Author: pango3001 <pango3001@gmail.com>
Date:   Mon Nov 2 13:34:46 2020 -0600

    Update oss.c

commit 16acad6cbc126069c8589818f4fdb05b14cf0ec2
Author: pango3001 <pango3001@gmail.com>
Date:   Mon Nov 2 08:45:17 2020 -0600

    clean up and comments

commit d983a199b95e673c2b8a2922a66e3c3a8e1f6ce5
Author: pango3001 <pango3001@gmail.com>
Date:   Mon Nov 2 00:51:12 2020 -0600

    Update oss.c

commit 84247d73d82cc19390dac756dc1e8b8063dae57b
Author: pango3001 <pango3001@gmail.com>
Date:   Mon Nov 2 00:50:17 2020 -0600

    Update oss.c

commit ed3fd1709e89d735f5447d6e900434966633d6be
Author: pango3001 <pango3001@gmail.com>
Date:   Mon Nov 2 00:47:40 2020 -0600

    Update oss.c

commit 4516945f752fda5f745ac1709973110d8c037c1a
Author: pango3001 <pango3001@gmail.com>
Date:   Mon Nov 2 00:46:57 2020 -0600

    Update oss.c

commit bfc2b3fcaf178a73e0f79305767ebd179d0e8257
Author: pango3001 <pango3001@gmail.com>
Date:   Mon Nov 2 00:44:39 2020 -0600

    Update user_proc.c

commit b7caa6061f6d298b997054e9c27408fd007d0b07
Author: pango3001 <pango3001@gmail.com>
Date:   Mon Nov 2 00:42:06 2020 -0600

    Update user_proc.c

commit 9ccafcc4b999214976404777747d0cb7ede660c0
Author: pango3001 <pango3001@gmail.com>
Date:   Mon Nov 2 00:38:11 2020 -0600

    Update user_proc.c

commit b38cd7284c0a8a33f31b0c0297e7a7057a2bbe1f
Author: pango3001 <pango3001@gmail.com>
Date:   Mon Nov 2 00:31:40 2020 -0600

    Update user_proc.c

commit dd9bafbad041158d6b234733cce46e4faa410823
Author: pango3001 <pango3001@gmail.com>
Date:   Mon Nov 2 00:26:14 2020 -0600

    Update oss.c

commit c2d8892202257f9e96ae3a760396220c7e266d4f
Author: pango3001 <pango3001@gmail.com>
Date:   Mon Nov 2 00:23:18 2020 -0600

    Update oss.c

commit 735cab47c5dc3e477a18b4a1ef0d129087a6cb7d
Author: pango3001 <pango3001@gmail.com>
Date:   Mon Nov 2 00:20:38 2020 -0600

    Update oss.c

commit c95d63e0d7ca65db538c6872028830d38a3c62b3
Author: pango3001 <pango3001@gmail.com>
Date:   Mon Nov 2 00:16:29 2020 -0600

    Update oss.c

commit 712852524dc23a37b63a2d999681afe81faaafe8
Author: pango3001 <pango3001@gmail.com>
Date:   Mon Nov 2 00:12:14 2020 -0600

    updates

commit 781615c82d126925b7b568648ff84e6c5edafa07
Author: pango3001 <pango3001@gmail.com>
Date:   Mon Nov 2 00:01:11 2020 -0600

    updates

commit 16086437b8ef088bbc4e1a519616af2b64073006
Author: pango3001 <pango3001@gmail.com>
Date:   Sun Nov 1 22:54:53 2020 -0600

    Update oss.c

commit bcdc82f05837f44a89d6cf861d14d8e09243a2c5
Author: pango3001 <pango3001@gmail.com>
Date:   Sun Nov 1 21:44:16 2020 -0600

    Update oss.c

commit 46c5495510f71dbc215be5322826210fd5aa2ad6
Author: pango3001 <pango3001@gmail.com>
Date:   Sun Nov 1 21:39:37 2020 -0600

    Update oss.c

commit 1ef7ae0b232aceaa26d90c69a4348b3b6c9f3ad5
Author: pango3001 <pango3001@gmail.com>
Date:   Sun Nov 1 21:36:57 2020 -0600

    Update oss.c

commit c6caeff82222612c220fa5214e96c911a35dd6e9
Author: pango3001 <pango3001@gmail.com>
Date:   Sun Nov 1 21:30:34 2020 -0600

    updated outputs

commit 8df68f138ca4c814bd19bd4bb39ec7a43f4b9010
Author: pango3001 <pango3001@gmail.com>
Date:   Sun Nov 1 20:16:45 2020 -0600

    clear screen added

commit 86de218d17fdb1e731d840703a8bf525d80b4194
Author: pango3001 <pango3001@gmail.com>
Date:   Sun Nov 1 20:13:24 2020 -0600

    Update oss.c

commit 687f8951f497e2a59c6be27b00129d9cb8b9edf1
Author: pango3001 <pango3001@gmail.com>
Date:   Sun Nov 1 20:07:05 2020 -0600

    Update oss.c

commit 7de595129b7c7175049df464cb04d11c203a0457
Author: pango3001 <pango3001@gmail.com>
Date:   Sun Nov 1 20:00:29 2020 -0600

    fixed seg fault and clean up

commit 3a6bdd4d05a7da4fb53a7a2840b9ffc14232a263
Author: pango3001 <pango3001@gmail.com>
Date:   Sun Nov 1 19:54:58 2020 -0600

    Update oss.c

commit cf6083bab9f8e3e31be7adada901103e45bd292c
Author: pango3001 <pango3001@gmail.com>
Date:   Sun Nov 1 19:52:34 2020 -0600

    Update oss.c

commit 830e49cba70041d4d1c5b4cf284d28714904252a
Author: pango3001 <pango3001@gmail.com>
Date:   Sun Nov 1 19:49:51 2020 -0600

    Fixed key issue

commit 7dd9b2c4a766e8a760709e19f19326247ec6bdd1
Author: pango3001 <pango3001@gmail.com>
Date:   Sun Nov 1 19:35:41 2020 -0600

    alot of cleanup

commit 2dd9ff7d27fdbc1610c625b35da99a69dad80e54
Author: pango3001 <pango3001@gmail.com>
Date:   Sun Oct 25 21:10:43 2020 -0500

    Update oss.c

commit d0705bd16c12b93a5722fb143b2d0f18c40b931d
Author: pango3001 <pango3001@gmail.com>
Date:   Sun Oct 25 20:36:20 2020 -0500

    Update oss.c

commit eee01d24ac454355a97f54920e9feec5f7099999
Author: Jesse McCarville <pango3001@gmail.com>
Date:   Sun Oct 25 17:41:04 2020 -0500

    fixed errors

commit d65527356534bbcb538c4094397611965fbcfc13
Author: pango3001 <pango3001@gmail.com>
Date:   Sun Oct 25 17:35:49 2020 -0500

    Update oss.c

commit e03778256c129bf3696591f225720213509afb59
Author: Jesse McCarville <pango3001@gmail.com>
Date:   Sun Oct 25 14:04:34 2020 -0500

    fixed small issues

commit 7a59b82d052b8799030115e63e9be81d745e2097
Author: pango3001 <pango3001@gmail.com>
Date:   Sun Oct 25 14:01:46 2020 -0500

    removed somethings (cleaned up)

commit 035e3e625cf059d587af85c48c2323c932e23e9e
Author: Jesse McCarville <pango3001@gmail.com>
Date:   Sun Oct 25 13:47:11 2020 -0500

    cleaned

commit aeead22dd270fa9e13ce3384fb51b5f421e814ef
Author: Jesse McCarville <pango3001@gmail.com>
Date:   Sat Oct 24 13:50:36 2020 -0500

    updates

commit d03bcebea3e037846724b742394daea2a2f98cc0
Author: pango3001 <pango3001@gmail.com>
Date:   Sat Oct 24 13:38:10 2020 -0500

    Update oss.c

commit 20da57b50751fcd936b6c4029b2c4b04e4298527
Author: pango3001 <pango3001@gmail.com>
Date:   Sat Oct 24 13:36:19 2020 -0500

    updated header files

commit 23c26ec88d0ff761aa7f0b2f9e3c460801876ec3
Merge: aff8027 e4684c6
Author: Jesse McCarville <pango3001@gmail.com>
Date:   Sat Oct 24 13:28:21 2020 -0500

    Merge branch 'main' of https://github.com/pango3001/process_scheduling into main

commit aff8027cf56ff6363d5966780f6db1cb9caa4661
Author: Jesse McCarville <pango3001@gmail.com>
Date:   Sat Oct 24 13:27:07 2020 -0500

    added header files

commit e4684c6805cbdc3fef3899c2f809a6812653298b
Author: pango3001 <pango3001@gmail.com>
Date:   Sat Oct 24 13:26:15 2020 -0500

    updates

commit 7ba5a5c3db16e738d7b107cc5a7846bdd725b73e
Author: Jesse McCarville <pango3001@gmail.com>
Date:   Sat Oct 24 13:15:33 2020 -0500

    fixed user_proc.c

commit 3bd990e53d018c87036dac9ea9b5ad79b7e43124
Author: Jesse McCarville <pango3001@gmail.com>
Date:   Sat Oct 24 11:21:14 2020 -0500

    created makefile and .c files

commit 033b845c5ec37c375251d9d076e16f0b6e1004dc
Author: pango3001 <pango3001@gmail.com>
Date:   Sat Oct 24 11:04:47 2020 -0500

    Initial commit
