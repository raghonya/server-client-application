#include <stdlib.h>

void    free_2d_array(char **arr)
{
	if (!arr)
		return ;
	for (int i = 0; arr[i]; ++i)
		free(arr[i]);
	free(arr);
}
