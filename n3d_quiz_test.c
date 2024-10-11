#include "cwolfmap/cwolfmap.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
	CWAudioInit();
	CWolfMap map;
	int err = 0;
	if (argc != 2)
	{
		printf("Usage: n3d_quiz_test <path to S3DNA folder>\n");
		err = 1;
		goto bail;
	}
	err = CWLoad(&map, argv[1], 0);
	if (err != 0)
	{
		goto bail;
	}

	for (int i = 0; i < map.nQuizzes; i++)
	{
		const CWN3DQuiz *quiz = &map.quizzes[i];
		printf("Q%d: %s\n", i + 1, quiz->question);
		for (int j = 0; j < quiz->nAnswers; j++)
		{
			const char *answer = quiz->answers[j];
			printf(
				"%c: %s (%s)\n", (char)j + 'A', answer,
				j == quiz->correctIdx ? "correct" : "wrong");
		}
	}

bail:
	CWFree(&map);
	CWAudioTerminate();
	return err;
}
