/***************************************************************************
 * Dr. Evil's Insidious Bomb, Version 1.0
 * Copyright 2002, Dr. Evil Incorporated. All rights reserved.
 *
 * LICENSE:
 *
 * Dr. Evil Incorporated (the PERPETRATOR) hereby grants you (the
 * VICTIM) explicit permission to use this bomb (the BOMB).  This is a
 * time limited license, which expires on the death of the VICTIM.
 * The PERPETRATOR takes no responsibility for damage, frustration,
 * insanity, bug-eyes, carpal-tunnel syndrome, loss of sleep, or other
 * harm to the VICTIM.  Unless the PERPETRATOR wants to take credit,
 * that is.  The VICTIM may not distribute this bomb source code to
 * any enemies of the PERPETRATOR.  No VICTIM may debug,
 * reverse-engineer, run "strings" on, decompile, decrypt, or use any
 * other technique to gain knowledge of and defuse the BOMB.  BOMB
 * proof clothing may not be worn when handling this program.  The
 * PERPETRATOR will not apologize for the PERPETRATOR's poor sense of
 * humor.  This license is null and void where the BOMB is prohibited
 * by law.
 ***************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include "support.h"
#include "phases.h"

/* 
 * Note to self: Remember to erase this file so my victims will have no
 * idea what is going on, and so they will all blow up in a
 * spectaculary fiendish explosion. -- Dr. Evil 
 */

FILE *infile;
void (*phase_funcs[7])(char *)={NULL,phase_1,phase_2,phase_3,phase_4,phase_5,phase_6};


void usage(){
	fprintf(stderr,"Required arguments missing!\n"); 
    fprintf(stderr,"Usage: ./bomb -t <total phases of the bomb> -x <selected phase Ids of the bomb> [-s <solution file>]\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr,"  -t <total phases of the bomb> How many phases are waiting for you to defuse?\n");
    fprintf(stderr,"  -x <selected phase Ids of the bomb> Please list the number ID of every phase.\n");
    fprintf(stderr,"  -s <solution file> Optional. The solution file includes the solution strings of the bomb\n");
}

/*Judge if each character is unique in a string */
int isUnique(char *str){
	int a[8],i=0;  
    memset(a, 0, sizeof(a));  
    int len = strlen(str);  
    for(; i < len; ++i)  
    {  
        char v = (int)str[i];  
        int idx = v/32, shift=v%32;  
        if(a[idx] & (1 << shift)) return 0;  
        a[idx] |= (1 << shift);  
    }  
    return 1;  
}

/*Delete all the spaces in a string */
char* delSpaces(char *str){
	int i=0,j=0,len=strlen(str); 
	char tmp[100];
    for(;i<len;i++){
		if(str[i]!=' ') tmp[j++]=str[i];
	}
    tmp[j-1]='\0'; 
    return tmp;  
}

int main(int argc, char *argv[])
{
    char *input;
	int	ch;
	int total,phases_len;
	char *phaseIds,*solution_file;
	int t_exists=0,x_exists=0,s_exists=0;
//	void (*phase)(char*)=null;

	/* When run with no arguments, show the usage */

	if (argc == 1) {  
		usage();
		exit(8);
    } 

	
	/* Test if students input the right phase amount and phase IDs arguments */
//	FILE *arguments_file;
//	if (!(arguments_file = fopen("PhaseID", "r"))) {
//		printf("%s: Error: Couldn't open %s\n", argv[0], "PhaseID");
//		exit(8);
//	}else{
////		char arguments[100];
//		char *arguments=(char *)malloc(100);
//		char arguments_input[100];
//		strcpy(arguments_input,argv[1]);
//		int i=2;
//		for(;i<argc;i++){
//			strcat(arguments_input,argv[i]);
//		}
//		printf("arguments_input:%s.\n",arguments_input);
//		while (!feof(arguments_file)) fgets(arguments,100,arguments_file);
////		fprintf(stderr,"arguments are:%s.\n",arguments);
//		strcpy(arguments,delSpaces(arguments));
//		fprintf(stderr,"arguments are:%s.\n",arguments);
//		if (strcmp(arguments,arguments_input)) {
//			printf("Error: The arguments is not correct. Please refer to %s\n", "PhaseID");
//			exit(8);
//		}
//	}

	/* In default condition, the bomb reads its input lines 
     * from standard input. */
	infile=stdin;

	/* When missing required arguments, show the warning messages */
	opterr=1;

	 while ((ch = getopt(argc,argv,"t:x:s::"))!=-1){
		switch(ch){
			case 't':
				t_exists=1;
				total=*optarg-0x30;
				if (!((strlen(optarg)==1)&&(total>=1)&&(total<=6)))	{
					fprintf(stderr,"-t needs a number argument which is between 1 and 6.\n");
					exit(8);
				}
				break;
			case 'x':
				x_exists=1;
				phases_len=strlen(optarg);
				phaseIds=optarg;
				break;
			case 's':
				s_exists=1;
				solution_file=optarg;
				break;
			default:
				usage();
				exit(8);
		}
	 } 

	if (!(t_exists&&x_exists)){
		usage();
		exit(8);
	}

	/* Judge if bombIds are right string */
	if(!isUnique(phaseIds)){
		fprintf(stderr,"Each phase Id must be unique.\n");
		exit(8);
	}

	if(phases_len!=total){
		fprintf(stderr,"-x needs %d phase Ids.\n",total);
		exit(8);
	}
	else{
		int i=0;
		for(;i<total;i++){
			phaseIds[i]-=0x30;
			if(!(phaseIds[i]>=1&&phaseIds[i]<=6)){
				fprintf(stderr,"phase Ids is between 1 and 6.\n");
				exit(8);
			}
		}
	}


  
    /* When run with one argument <file>, the bomb reads from <file> 
     * until EOF, and then switches to standard input. Thus, as you 
     * defuse each phase, you can add its defusing string to <file> and
     * avoid having to retype it. */
	if (s_exists){
//		printf(solution_file);
		if (!(infile = fopen(solution_file, "r"))) {
				printf("%s: Error: Couldn't open %s\n", argv[0], solution_file);
				exit(8);
			}
	}	

	
    /* Do all sorts of secret stuff that makes the bomb harder to defuse. */
    initialize_bomb();

    printf("Welcome to my fiendish little bomb. You have %d phases with\n",total);
    printf("which to blow yourself up. Have a nice day!\n");

	int defused=0;
	for(;defused<total;defused++){
		input=read_line(phaseIds[defused]);
/*		switch (phaseIds[defused]) {
			case 1:
				phase_1(input);
				break;
			case 2:
				phase_2(input);
				break;
			case 3:
				phase_3(input);
				break;
			case 4:
				phase_4(input);
				break;
			case 5:
				phase_5(input);
				break;
			case 6:
				phase_6(input);
				break;
			default:
				fprintf(stderr,"Secret bomb.\n");		
		}*/
		phase_funcs[phaseIds[defused]](input);
		phase_defused(phaseIds[defused]);

		printf("The bomb phase %d is defused. Congratulations!\n",phaseIds[defused]);
	}
	
	
	
	/* Hmm...  Six phases must be more secure than one phase! */
//    input = read_line();             /* Get input                   */
//    phase_0(input);                  /* Run the phase               */
//    phase_defused();                 /* Drat!  They figured it out!
//				      * Let me know how they did it. */
//    printf("Phase 1 defused. How about the next one?\n");
//    printf("The bomb is defused. Congratulations!\n");


    /* The second phase is harder.  No one will ever figure out
     * how to defuse this... 
    input = read_line();
    phase_2(input);
    phase_defused();
    printf("That's number 2.  Keep going!\n");liusx*/


    /* I guess this is too easy so far.  Some more complex code will
     * confuse people. 
    input = read_line();
    phase_3(input);
    phase_defused();
    printf("Halfway there!\n");liusx*/


    /* Oh yeah?  Well, how good is your math?  Try on this saucy problem! 
    input = read_line();
    phase_4(input);
    phase_defused();
    printf("So you got that one.  Try this one.\n");liusx*/

    
    /* Round and 'round in memory we go, where we stop, the bomb blows! 
    input = read_line();
    phase_5(input);
    phase_defused();
    printf("Good work!  On to the next...\n");liusx*/


    /* This phase will never be used, since no one will get past the
     * earlier ones.  But just in case, make this one extra hard. 
    input = read_line();
    phase_6(input);
    phase_defused();liusx*/

    /* Wow, they got it!  But isn't something... missing?  Perhaps
     * something they overlooked?  Mua ha ha ha ha! */
    
    return 0;
}
