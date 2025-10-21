#include "sc.h"

void	lstadd_back(list_t **lst, list_t *new)
{
	list_t	*a;

	if (!*lst && new)
	{
		*lst = new;
		return ;
	}
	else if (!new)
		return ;
	a = *lst;
	while ((*lst)->next)
		*lst = (*lst)->next;
	(*lst)->next = new;
	*lst = a;
}

void	lstadd_front(list_t **lst, list_t *new)
{
	if (!*lst && new)
	{
		*lst = new;
		return ;
	}
	else if (!new)
		return ;
	if (lst && new)
	{
		new->next = *lst;
		*lst = new;
	}
}

void	lstclear(list_t **lst)
{
	list_t	*t;

	if (!lst)
		return ;
	while (*lst)
	{
		t = (*lst)->next;
		lstdelone(*lst);
		*lst = t;
	}
	*lst = NULL;
}

void	lstdelone(list_t *lst)
{
	free (lst);
}

// void	lstremove_elem(list_t **lst, int sock)
// {
// 	list_t	*tmp;
// 	list_t	*start;


// 	if (!lst)
// 		return ;
// 	start = *lst;	
// 	while (*lst)
// 	{
// 		tmp = (*lst)->next;
// 		if ((*lst)->content == sock)
// 		{

// 			lstdelone(*lst);
			
// 		}
// 		*lst = tmp;
// 	}
// }

list_t	*lstnew(int content)
{
	list_t	*new;

	new = malloc(sizeof(list_t));
	if (!new)
		return (NULL);
	new->content = content;
	new->next = NULL;
	return (new);
}

int	lstsize(list_t *lst)
{
	int	i;

	i = 0;
	while (lst != NULL)
	{
		i++;
		lst = lst->next;
	}
	return (i);
}
