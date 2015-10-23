#include <mpi.h>
#include <assert.h>
#include <cstdio>
#include <iostream>
#include <string>

const int BUF_SIZE = 8192;

using namespace std;

int main(int argc, char* argv[]) {

    int num_tasks, rank, dest, source, tag;
    char buf[BUF_SIZE];

    // initialization
    tag = 1;
    MPI_Status Stat;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &num_tasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // 0 sending, 1 receiving
    if (rank == 0) {
        dest = 1;
        int cnt = sprintf(buf, "Here is the message!");
        int rc = MPI_Send(buf, cnt, MPI_CHAR, dest, tag, MPI_COMM_WORLD);
        assert (rc == MPI_SUCCESS);
    } else if (rank == 1) {
        source = 0;
        int rc = MPI_Recv(buf, BUF_SIZE, MPI_CHAR, source, tag, MPI_COMM_WORLD, &Stat);
        assert (rc == MPI_SUCCESS);
        int count = 0;
        rc = MPI_Get_count(&Stat, MPI_CHAR, &count);
        string str(buf, count);
        cout << str << endl;
    }

    MPI_Finalize();
    return 0;
}
