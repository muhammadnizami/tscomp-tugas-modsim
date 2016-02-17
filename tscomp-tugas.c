//BIG TODO: PERIKSA SEMUA

/* External definitions for time-shared computer model. */

#include "simlib.h"               /* Required for use of simlib.c. */

#define EVENT_ARRIVAL          1  /* Event type for arrival of job to CPU. */
#define EVENT_END_CPU_1_RUN    2  /* Event type for end of a CPU run. */
#define EVENT_END_CPU_2_RUN    3  /* Event type for end of a CPU run. */
#define EVENT_END_CPU_3_RUN    4  /* Event type for end of a CPU run. */
#define EVENT_END_SIMULATION   5  /* Event type for end of the simulation. */
#define LIST_QUEUE_1           1  /* List number for CPU queue. */
#define LIST_QUEUE_2           2  /* List number for CPU queue. */
#define LIST_QUEUE_3           3  /* List number for CPU queue. */
#define LIST_CPU_1             4  /* List number for CPU. */
#define LIST_CPU_2             5  /* List number for CPU. */
#define LIST_CPU_3             6  /* List number for CPU. */
#define SAMPST_RESPONSE_TIMES_1 1
#define SAMPST_RESPONSE_TIMES_2 2
#define SAMPST_RESPONSE_TIMES_3 3
#define SAMPST_RESPONSE_TIMES  4  /* sampst variable for response times. */
#define STREAM_THINK           1  /* Random-number stream for think times. */
#define STREAM_SERVICE         2  /* Random-number stream for service times. */

/* Declare non-simlib global variables. */

int   min_terms, max_terms, incr_terms, num_terms, num_responses,
      num_responses_required, term;
//bagi2 job. TODO ganti sama input
int batasatasjob1 = 10;
int batasatasjob2 = 40;
float mean_think, mean_service, quantum, swap;
FILE  *infile, *outfile;

/* Declare non-simlib functions. */

void arrive(void);
void start_CPU_run(int jenis_job_queue);
void end_CPU_run(int list_cpu_num);
void report(void);


int main()  /* Main function. */
{
    /* Open input and output files. */

    infile  = fopen("tscomp-tugas.in",  "r");
    outfile = fopen("tscomp-tugas.out", "w");

    /* Read input parameters. */

    fscanf(infile, "%d %d %d %d %f %f %f %f",
           &min_terms, &max_terms, &incr_terms, &num_responses_required,
           &mean_think, &mean_service, &quantum, &swap);

    /* Write report heading and input parameters. */

    fprintf(outfile, "Time-shared computer model\n\n");
    fprintf(outfile, "Number of terminals%9d to%4d by %4d\n\n",
            min_terms, max_terms, incr_terms);
    fprintf(outfile, "Mean think time  %11.3f seconds\n\n", mean_think);
    fprintf(outfile, "Mean service time%11.3f seconds\n\n", mean_service);
    fprintf(outfile, "Quantum          %11.3f seconds\n\n", quantum);
    fprintf(outfile, "Swap time        %11.3f seconds\n\n", swap);
    fprintf(outfile, "Number of jobs processed%12d\n\n\n",
            num_responses_required);
    fprintf(outfile, "Number of      Average         Average");
    fprintf(outfile, "       Utilization      CPU-num/\n");
    fprintf(outfile, "terminals   response time  number in queue     of CPU          JOB-type");

    /* Run the simulation varying the number of terminals. */

    for (num_terms = min_terms; num_terms <= max_terms;
         num_terms += incr_terms) {

        /* Initialize simlib */

        init_simlib();

        /* Set maxatr = max(maximum number of attributes per record, 4) */

        maxatr = 4;  /* NEVER SET maxatr TO BE SMALLER THAN 4. */

        /* Initialize the non-simlib statistical counter. */

        num_responses = 0;

        /* Schedule the first arrival to the CPU from each terminal. */

        for (term = 1; term <= num_terms; ++term)
            event_schedule(expon(mean_think, STREAM_THINK), EVENT_ARRIVAL);

        /* Run the simulation until it terminates after an end-simulation event
           (type EVENT_END_SIMULATION) occurs. */

        do {

            /* Determine the next event. */

            timing();

            /* Invoke the appropriate event function. */

            switch (next_event_type) {
                case EVENT_ARRIVAL:
                    arrive();
                    break;
                case EVENT_END_CPU_1_RUN:
                    end_CPU_run(LIST_CPU_1);
                    break;
                case EVENT_END_CPU_2_RUN:
                    end_CPU_run(LIST_CPU_2);
                    break;
                case EVENT_END_CPU_3_RUN:
                    end_CPU_run(LIST_CPU_3);
                    break;
                case EVENT_END_SIMULATION:
                    report();
                    break;
            }

        /* If the event just executed was not the end-simulation event (type
           EVENT_END_SIMULATION), continue simulating.  Otherwise, end the
           simulation. */

        } while (next_event_type != EVENT_END_SIMULATION);
    }

    fclose(infile);
    fclose(outfile);

    return 0;
}

void arrive(void)  /* Event function for arrival of job at CPU after think
                      time. */
{

    /* Place the arriving job at the end of the CPU queue.
       Note that the following attributes are stored for each job record:
            1. Time of arrival to the computer.
            2. The (remaining) CPU service time required (here equal to the
               total service time since the job is just arriving).*/

    transfer[1] = sim_time;
    transfer[2] = expon(mean_service, STREAM_SERVICE);//TODO nanti ganti sama distribusi yang disebut di spek (belum jelas)
    //jenis job
    int jenis_job;
    if (transfer[2]<=batasatasjob1)
        jenis_job=LIST_QUEUE_1;
    else if (transfer[2]<=batasatasjob2)
        jenis_job=LIST_QUEUE_2;
    else
        jenis_job=LIST_QUEUE_3;
    list_file(LAST, jenis_job); 

    /* If the CPU is idle, start a CPU run. */
    switch(jenis_job){
        case LIST_QUEUE_1:
        if (list_size[LIST_CPU_1] == 0)
            start_CPU_run(jenis_job);
        break;
        case LIST_QUEUE_2:
        if (list_size[LIST_CPU_2] == 0)
            start_CPU_run(jenis_job);
        break;
        case LIST_QUEUE_3:
        if (list_size[LIST_CPU_3] == 0)
            start_CPU_run(jenis_job);
        break;
    }
}


void start_CPU_run(int jenis_job_queue)  /* Non-event function to start a CPU run of a job. */
{
    float run_time;

    /* Remove the first job from the queue. */

    list_remove(FIRST, jenis_job_queue);

    /* Determine the CPU time for this pass, including the swap time. */

    if (quantum < transfer[2])
        run_time = quantum + swap;
    else
        run_time = transfer[2] + swap;

    /* Decrement remaining CPU time by a full quantum.  (If less than a full
       quantum is needed, this attribute becomes negative.  This indicates that
       the job, after exiting the CPU for the current pass, will be done and is
       to be sent back to its terminal.) */

    transfer[2] -= quantum;

    /* Place the job into the CPU. */

    /* Schedule the end of the CPU run. */
    switch(jenis_job_queue){
        case LIST_QUEUE_1:
            list_file(FIRST, LIST_CPU_1);

	    event_schedule(sim_time + run_time, EVENT_END_CPU_1_RUN);
        break;
        case LIST_QUEUE_2:
            list_file(FIRST, LIST_CPU_2);

	    event_schedule(sim_time + run_time, EVENT_END_CPU_2_RUN);
        break;
        case LIST_QUEUE_3:
            list_file(FIRST, LIST_CPU_3);

	    event_schedule(sim_time + run_time, EVENT_END_CPU_3_RUN);
        break;
    }
}


void end_CPU_run(int list_cpu_num)  /* Event function to end a CPU run of a job. */
{
    /* Remove the job from the CPU. */

    list_remove(FIRST, list_cpu_num);

    int jenis_job_queue;
    switch (list_cpu_num){
    case LIST_CPU_1:
        jenis_job_queue=LIST_QUEUE_1;
    break;
    case LIST_CPU_2:
        jenis_job_queue=LIST_QUEUE_2;
    break;
    case LIST_CPU_3:
        jenis_job_queue=LIST_QUEUE_3;
    break;
    }

    /* Check to see whether this job requires more CPU time. */

    if (transfer[2] > 0.0) {

        /* This job requires more CPU time, so place it at the end of the queue
           and start the first job in the queue. */

        list_file(LAST, jenis_job_queue);
        start_CPU_run(jenis_job_queue);
    }

    else {

        /* This job is finished, so collect response-time statistics and send it
           back to its terminal, i.e., schedule another arrival from the same
           terminal. */

        sampst(sim_time - transfer[1], SAMPST_RESPONSE_TIMES);//TODO nanti hapus
        switch(jenis_job_queue){
        case (LIST_QUEUE_1):
            sampst(sim_time - transfer[1], SAMPST_RESPONSE_TIMES_1);
        break;
        case (LIST_QUEUE_2):
            sampst(sim_time - transfer[1], SAMPST_RESPONSE_TIMES_2);
        break;
        case (LIST_QUEUE_3):
            sampst(sim_time - transfer[1], SAMPST_RESPONSE_TIMES_3);
        break;
        }
	

        event_schedule(sim_time + expon(mean_think, STREAM_THINK),
                       EVENT_ARRIVAL); //TODO nanti ganti sama distribusi yang disebut di spek (belum jelas)

        /* Increment the number of completed jobs. */

        ++num_responses;

        /* Check to see whether enough jobs are done. */

        if (num_responses >= num_responses_required)

            /* Enough jobs are done, so schedule the end of the simulation
               immediately (forcing it to the head of the event list). */

            event_schedule(sim_time, EVENT_END_SIMULATION);

        else

            /* Not enough jobs are done; if the queue is not empty, start
               another job. */

            if (list_size[jenis_job_queue] > 0)
                start_CPU_run(jenis_job_queue);
    }
}


void report(void)  /* Report generator function. */
{
    /* Get and write out estimates of desired measures of performance. */

    fprintf(outfile, "\n\n%5d%16.3f%16.3f%16.3f         CPU-1", num_terms,
            sampst(0.0, -SAMPST_RESPONSE_TIMES_1), filest(LIST_QUEUE_1),
            filest(LIST_CPU_1));
    fprintf(outfile, "\n     %16.3f%16.3f%16.3f         CPU-2", 
            sampst(0.0, -SAMPST_RESPONSE_TIMES_2), filest(LIST_QUEUE_2),
            filest(LIST_CPU_2));
    fprintf(outfile, "\n     %16.3f%16.3f%16.3f         CPU-3\n", 
            sampst(0.0, -SAMPST_RESPONSE_TIMES_3), filest(LIST_QUEUE_3),
            filest(LIST_CPU_3));
}

