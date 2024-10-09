#include "cwolfmap/n3d.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
	char *languageBuf = NULL;
	int err = 0;
	if (argc != 2)
	{
		printf("Usage: n3d_quiz_test <path to noah3d.pk3>\n");
		err = 1;
		goto bail;
	}
	languageBuf = CWN3DLoadLanguageEnu(argv[1]);
	if (languageBuf == NULL)
	{
		err = 1;
		goto bail;
	}

	for (int quiz = 1;; quiz++)
	{
		char *question = CWLevelN3DLoadQuizQuestion(languageBuf, quiz);
		if (question == NULL)
		{
			break;
		}
		printf("Q%d: %s\n", quiz, question);
		for (char a = 'A';; a++)
		{
			bool correct = false;
			char *answer =
				CWLevelN3DLoadQuizAnswer(languageBuf, quiz, a, &correct);
			if (answer == NULL)
			{
				break;
			}
			printf("%c: %s (%s)\n", a, answer, correct ? "correct" : "wrong");
			free(answer);
		}
		free(question);
	}

bail:
	return err;
}
