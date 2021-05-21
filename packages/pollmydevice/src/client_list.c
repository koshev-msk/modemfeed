#include <stdio.h>
#include <stdlib.h>

#include "client_list.h"

struct client_list {
    int descr;
    struct client_list *next;
};

struct client_list *head = NULL;
struct client_list *request = NULL;

struct client_list * get_end_list()
{
    struct client_list * current = head;
    if (current == NULL)
    {
        return NULL;
    }
    while (current->next != NULL)
    {
        current=current->next;
    }
    return current;
    
}

void client_insert(int new_descr)
{ 
    struct client_list *end=NULL;
    if (head == NULL)
    {
        head = malloc(sizeof(struct client_list));
        if (head == NULL){
            return;
        }
        head->descr = new_descr;
        head->next = NULL;
        return;
    }
    end = get_end_list();
    if (end==NULL){
        return;
    }
    struct client_list *new_client =(struct client_list *) malloc(sizeof(struct client_list));
    if(new_client==NULL)
    {
        return;
    }
    new_client->descr = new_descr;
    new_client->next = NULL;
    end->next = new_client;
}

void start_request_descr()
{
    request=head;
}

int get_next_descr(int *descr)
{
    if(request==NULL)
    {
        return 1;
    }
    *descr=request->descr;
    request=request->next;
    return 0;
}

void remove_descr(int descr)
{
    struct client_list *current = head;
    struct client_list *prev = NULL;
    if(current==NULL)
    {
        return;
    }


    while(current->descr!=descr)
    {
        prev=current;
        current=current->next;
        if(current==NULL)
        {
            return;
        }
    }
    free(current);
    if(current!=head){
        prev->next=current->next;
    } else {
    	head=head->next;
    }    
    start_request_descr();

}

