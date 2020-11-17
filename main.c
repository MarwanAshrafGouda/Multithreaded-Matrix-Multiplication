#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/time.h>

struct arg_struct {
    double *first_input_matrix; // pointer to the first element of the first matrix
    double *second_input_matrix; // pointer to the first element of the second matrix
    double *output_matrix; // pointer to the first element of the output matrix
    int row; // the row of the calculated element
    int col; // the column of the calculated element
    int cols_1; // number of columns of the first matrix
    int cols_2; // number of columns of the second matrix
    int out_cols; // number of columns of the output matrix
};

void *matrix_multiplication_element(void *args) {
    struct arg_struct *arg_ptr = (struct arg_struct *) args;
    for (int i = 0; i < arg_ptr->cols_1; i++) {
        *((arg_ptr->output_matrix + arg_ptr->row * arg_ptr->out_cols) + arg_ptr->col) +=
                *((arg_ptr->first_input_matrix + arg_ptr->row * arg_ptr->cols_1) + i) *
                *((arg_ptr->second_input_matrix + arg_ptr->cols_2 * i) + arg_ptr->col);
    }
    return NULL;
}

void *matrix_multiplication_row(void *args) {
    struct arg_struct *arg_ptr = (struct arg_struct *) args;
    for (int column = 0; column < arg_ptr->out_cols; column++) {
        for (int i = 0; i < arg_ptr->cols_1; i++) {
            *((arg_ptr->output_matrix + arg_ptr->row * arg_ptr->out_cols) + column) +=
                    *((arg_ptr->first_input_matrix + arg_ptr->row * arg_ptr->cols_1) + i) *
                    *((arg_ptr->second_input_matrix + arg_ptr->cols_2 * i) + column);
        }
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    FILE *first_input_file, *second_input_file, *output_file;

    // Setting default values for the input file names
    first_input_file = fopen((argc == 4) ? argv[1] : "a.txt", "r");
    second_input_file = fopen((argc == 4) ? argv[2] : "b.txt", "r");

    // Checking if all the input files exist, terminates if one file doesn't exist
    if (!(first_input_file && second_input_file)) {
        printf("File does not exist\n");
        return 0;
    }

    // Setting default value for the output file name
    output_file = fopen((argc == 4) ? argv[3] : "c.out", "w");

    // Reading the input matrices dimensions and checking the ability to perform matrix multiplication
    int rows_1, columns_1, rows_2, columns_2;
    fscanf(first_input_file, "row=%d col=%d\n", &rows_1, &columns_1);
    fscanf(second_input_file, "row=%d col=%d\n", &rows_2, &columns_2);
    if (columns_1 != rows_2) {
        printf("In order to perform matrix multiplication, the number of columns of the first"
               "matrix must be equal to the number of rows of the second matrix.\nThis condition was not met.\n"
               "Terminating.");
        return 0;
    }

    // Reading the input matrices, terminates if the given number of rows and columns don't match the actual dimensions
    double first_input_matrix[rows_1][columns_1];
    double second_input_matrix[rows_2][columns_2];
    for (int i = 0; i < rows_1; i++) {
        for (int j = 0; j < columns_1; j++) {
            if (fscanf(first_input_file, "%lf", &first_input_matrix[i][j]) == EOF) {
                printf("Incorrect Dimensions in the First Matrix\n");
                return 0;
            }
        }
    }
    for (int i = 0; i < rows_2; i++) {
        for (int j = 0; j < columns_2; j++) {
            if (fscanf(second_input_file, "%lf", &second_input_matrix[i][j]) == EOF) {
                printf("Incorrect Dimensions in the Second Matrix\n");
                return 0;
            }
        }
    }

    double output_matrix[rows_1][columns_2];
    // Initializing the output matrix with zeroes to facilitate the multiplication
    for (int i = 0; i < rows_1; i++) {
        for (int j = 0; j < columns_2; j++) {
            output_matrix[i][j] = 0;
        }
    }

    // 2D array of structs, each struct containing the arguments of the corresponding thread_element in order to prevent a race condition
    struct arg_struct args[rows_1][columns_2];
    for (int i = 0; i < rows_1; i++) {
        for (int j = 0; j < columns_2; j++) {
            args[i][j].first_input_matrix = (double *) first_input_matrix;
            args[i][j].second_input_matrix = (double *) second_input_matrix;
            args[i][j].output_matrix = (double *) output_matrix;
            args[i][j].row = i;
            args[i][j].col = j;
            args[i][j].cols_1 = columns_1;
            args[i][j].cols_2 = columns_2;
            args[i][j].out_cols = columns_2;
        }
    }

    // Calculating the output matrix using a thread_element for each element
    struct timeval stop, start;
    gettimeofday(&start, NULL); //start checking time
    pthread_t thread_element[rows_1 * columns_2];
    int thread_count = 0; // Counts the number of created threads
    int thread_increment = 0; // Increments the index of the thread_element to be created
    for (int i = 0; i < rows_1; i++) {
        for (int j = 0; j < columns_2; j++) {
            int error = pthread_create(&thread_element[thread_increment++], NULL, matrix_multiplication_element,
                                       (void *) &args[i][j]);
            if (error) { // If pthread_create didn't return NULL
                printf("Couldn't Create Thread");
                return 0;
            }
            thread_count++;
        }
    }
    for (int i = 0; i < rows_1 * columns_2; i++)
        pthread_join(thread_element[i], NULL); // Prevents main() from terminating before the threads are done executing
    gettimeofday(&stop, NULL); //end checking time
    for (int i = 0; i < rows_1; i++) {
        for (int j = 0; j < columns_2; j++) {
            fprintf(output_file, "%.2lf\t", output_matrix[i][j]);
        }
        fprintf(output_file, "\n");
    }
    printf("First Method: A Thread for Each Element\nThreads Created: %d\nMicroseconds Taken: %lu\n", thread_count,
           stop.tv_usec - start.tv_usec);

    // Reinitializing the output matrix for debugging purposes
    for (int i = 0; i < rows_1; i++) {
        for (int j = 0; j < columns_2; j++) {
            output_matrix[i][j] = 0;
        }
    }

    // Calculating the output matrix using a thread_element for each row
    gettimeofday(&start, NULL);
    pthread_t thread_row[rows_1];
    thread_count = 0;
    thread_increment = 0;
    for (int i = 0; i < rows_1; i++) {
        int error = pthread_create(&thread_row[thread_increment++], NULL, matrix_multiplication_row,
                                   (void *) &args[i][0]);
        if (error) {
            printf("Couldn't Create Thread");
            return 0;
        }
        thread_count++;
    }
    for (int i = 0; i < rows_1; i++)
        pthread_join(thread_row[i], NULL);
    gettimeofday(&stop, NULL);
    printf("\nSecond Method: A Thread for Each Row\nThreads Created: %d\nMicroseconds Taken: %lu\n", thread_count,
           stop.tv_usec - start.tv_usec);
    return 0;
}
