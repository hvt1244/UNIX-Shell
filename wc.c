#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define BUFSIZE 1000
int main(int argc, char *argv[]) {
int words=0, lines=0, bytes=0;
FILE *fp;
fp = fopen(argv[1],"r");

if (fp==NULL)
	printf (stderr,"cat: can`t open %s\n",argv[1]);

char buff[BUFSIZE];
char* s;
while (fgets(buff,BUFSIZE-1,fp) != NULL)
{
	int i=0,s=0;
	lines++;
	//while (buff>>s)
	//	words++;
	s = strlen(buff);
   	//printf ("String %s\n", buff);
	while (buff[i]!='\0' || i<s)
	   {
		bytes++;
		//printf ("%d\n",i);
		if (buff[i] == ' ')
			words++;
		i++;
	}
	
//bytes++;

}
words = words+lines;//-6*lines;
printf ("%d\t %d\t %d\t %s\n", lines, words, bytes, argv[1]);


    return 0;
}
