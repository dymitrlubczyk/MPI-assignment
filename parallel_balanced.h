
void master(int N, int R, int node_count);
void worker(int N, int R, int node_count, int id);
int get_stop();
int send_stop(int stop);
int get_results(int node_count);
void send_result(result);
int *get_task(int task_size);
int send_tasks(int *A, int N, int node_count);
int *initialise(int N);