#include<iostream>
#include<sys/wait.h>
#include<pthread.h>
#include<time.h>
#include<stdio.h>
#include<string.h>
#include<unistd.h>


#define UNPROCESSED 0
#define PROCESSED 1


using namespace std;

int **M;
int *A;
int *B;

int rows = 0, cols = 0;
pthread_mutex_t mat_mutex;
pthread_mutex_t A_mutex;
pthread_mutex_t B_mutex;
pthread_cond_t mat_cond = PTHREAD_COND_INITIALIZER;

void *order_func(void *arg){
	int thread_id = *(int *)arg;

	cout << "I am order "<< thread_id<<"!"<<endl;

	int itr = 30;
	while (itr > 0){

		int r, c;
		pthread_mutex_lock(&A_mutex);
		for (int i = 0; i < 1000; i++){
			if(A[i] >= 0){
				r = A[i];
				A[i] = -1;
				break;
			}
		}
		pthread_mutex_lock(&B_mutex);
		B[r] = PROCESSED;
		pthread_mutex_unlock(&B_mutex);
		pthread_mutex_unlock(&A_mutex);
		cout <<" Order "<< thread_id<<" detected updated row element in row "<<r<<endl;
		
		int arr[cols], arr1[cols];
		pthread_mutex_lock(&mat_mutex);
		for (int i = 0; i < cols; i++){
			arr[i] = M[r][i];
		}
		for (int i = 0; i < cols; i++){
			for (int j = i+1; j < cols; j++){
				if (M[r][i] > M[r][j]){
					int temp = M[r][i];
					M[r][i] = M[r][j];
					M[r][j] = temp;
				}
			}
		}
		for (int i = 0; i < cols; i++){
			arr1[i] = M[r][i];
		}
		pthread_mutex_unlock(&mat_mutex);
		cout << "Order "<< thread_id<<": row "<<r<<" is sorted!" <<endl;
		cout<<"Old row:" <<endl;
		for (int i = 0; i < cols; i++){
			cout<<arr[i]<<" ";
		}
		cout <<endl;

		cout<<"New row:" <<endl;
		for (int i = 0; i < cols; i++){
			cout<<arr1[i]<<" ";
		}
		cout <<endl;
		itr--;
	}
	pthread_exit(0);
}


void *chaos_func(void *arg){
	
	int itr = 30;

	cout << "I am chaos!"<<endl;
	while(itr > 0){
		int rand_r = (rand()% rows) + 1;
		int rand_c = (rand()% cols) + 1;

		int num = (rand()%1000) + 1;

		pthread_mutex_lock(&mat_mutex);
		M[rand_r][rand_c] = num;
		pthread_mutex_unlock(&mat_mutex);
		cout << "Chaos: updated elem at cell "<<rand_r<<", "<<rand_c<<" with val "<<num<<endl;

		pthread_mutex_lock(&A_mutex);
		for (int i = 0; i < 1000; i++){
			if(A[i] == -1){
				A[i] = rand_r;
				break;
			}
		}
		
		pthread_mutex_lock(&B_mutex);
		B[rand_r] = UNPROCESSED;
		pthread_mutex_unlock(&B_mutex);
		pthread_mutex_unlock(&A_mutex);

		itr--;
		sleep(2);
	}

	pthread_exit(0);
}

int main(){

	srand(time(NULL));


	int m; int n;
	cout << "Enter m and n:" << endl;
	cin >> m >> n;
	cout << "m = "<<m<<", n = "<<n<<endl;
	rows = m;cols = n;


	M = (int **)malloc(m*sizeof(int *));
	for (int i = 0; i < m; i++){
		M[i] = (int *)malloc(n*sizeof(int));
	}
	for (int i = 0; i < m; i++){
		for (int j = 0; j < n; j++){
			M[i][j] = (rand()%1000) + 1;
			// cout << M[i][j];
		}
	}

	cout << "Random  matrix M created!" << endl;

	A = (int *)malloc(1000*sizeof(int));
	B = (int *)malloc(1000*sizeof(int));
	for (int i = 0; i < 1000; i++){
		A[i] = -1; 
		B[i] = -1;
	}

	cout << "Shared arrays A and B created!" << endl;

	for (int i = 0; i < m; i++){
		for (int j = 0; j < n; j++){
			cout << M[i][j] << " ";
		}
		cout << endl;
	}
	cout << endl;

	pthread_mutex_init(&mat_mutex, NULL);
	pthread_mutex_init(&A_mutex, NULL);
	pthread_mutex_init(&B_mutex, NULL);

	pthread_t order[3], chaos;

	for (int i = 0; i < 3; i++){
		pthread_create(&order[i], NULL, order_func, &i);
	}

	pthread_create(&chaos, NULL, chaos_func, NULL);

	for (int i = 0; i < 3; i++){
		pthread_join(order[i], NULL);
	}

	pthread_join(chaos, NULL);
	return 0;

}
