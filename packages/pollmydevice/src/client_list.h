#ifndef _CLIENT_LIST_H
#define _CLIENT_LIST_H

void start_request_descr();
void client_insert(int new_descr);
int get_next_descr(int *descr);
void remove_descr(int descr);

#endif