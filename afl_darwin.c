#include "afl_darwin.h"
#include "rand.h"
#include <stdio.h>

unsigned MutationOperatorsNum;

#ifdef REAL_VALUED
	#define VALUE_TYPE double
#else
	#define VALUE_TYPE bool
#endif

#define M 20u
#define N 4u
#define LEARNING_RATE 0.3

int **_Fitness;
unsigned int **_bestIndex;
unsigned int *_nextToEvaluate;  // next to evaluate

VALUE_TYPE ***_individual;
VALUE_TYPE **_currentRelativeProb;

double **p;

/*
 * Initialize data structures for DARWIN
 * @nr_seeds: number of different initial seeds
 * @nr_mutations: number of mutation operators
*/
void DARWIN_init(uint64_t nr_seeds, unsigned nr_mutations) {
    // init RNG
	rand_init();

	MutationOperatorsNum = nr_mutations;

	// initialize opt alg data structures
    _Fitness = (int **) malloc(nr_seeds * sizeof(int *));
    _bestIndex = (unsigned int **) malloc(nr_seeds * sizeof(unsigned int *));
    _nextToEvaluate = (unsigned int *) malloc(nr_seeds * sizeof(unsigned int));

    _individual = (VALUE_TYPE ***) malloc(nr_seeds * sizeof(VALUE_TYPE **));

    _currentRelativeProb = (VALUE_TYPE **) malloc(nr_seeds * sizeof(VALUE_TYPE *));

    p = (double **) malloc(nr_seeds * sizeof(double *));

    printf("population size: %u, dominant individuals count %u\n", M, N);

    for (int seed = 0; seed < nr_seeds; seed++) {
        _nextToEvaluate[seed] = 0;

        _Fitness[seed] = (int *)malloc(M * sizeof(int));
        _bestIndex[seed] = (unsigned int *)malloc(N * sizeof(unsigned int));

        p[seed] = (double *)malloc(MutationOperatorsNum * sizeof(double));

        _individual[seed] = (VALUE_TYPE **) malloc(M * sizeof(VALUE_TYPE *));
		for (int i = 0; i < M; i++)
			_individual[seed][i] = (VALUE_TYPE *)malloc(MutationOperatorsNum * sizeof(VALUE_TYPE));

		memset(_Fitness[seed], 0, M);

		for(uint i = 0; i < MutationOperatorsNum; i++) {
		    p[seed][i] = 0.5;
		}

		for(uint i = 0; i < N; i++){
	        _bestIndex[seed][i] = 5 * i;
	    }

		// initial random values for individual
		for(uint i = 0; i < MutationOperatorsNum; i++) {
			// TODO: optionally replace with an alternate randomizer
			_individual[seed][0][i] = rand() > (RAND_MAX / 2);
		}

		_currentRelativeProb[seed] = _individual[seed][_nextToEvaluate[seed]];

    }
}

/*
 * Choose an AFL mutation operator
 * @seed: seed to select per-seed vector
*/
int DARWIN_SelectOperator(uint64_t seed)
{

	// select a random mutation operator with flag == true
	// revert to random if all false
	uint operatorId = rand_32_int(MutationOperatorsNum);
	uint nTries = 0;
	while (_currentRelativeProb[seed][operatorId] == false && nTries < MutationOperatorsNum) {
		nTries++;
		operatorId = (operatorId + 1) % MutationOperatorsNum;
	}
    return operatorId;
}


/*
 * Report feedback to DARWIN
 * @seed: seed to attribute this to
 * @numPaths: number of new paths
*/
void DARWIN_NotifyFeedback(uint64_t seed, unsigned numPaths)
{
	// update this candidate solution fitness
	_Fitness[seed][_nextToEvaluate[seed]] = numPaths;

	// update index[]
	if(_Fitness[seed][_nextToEvaluate[seed]] > _Fitness[seed][_bestIndex[seed][_nextToEvaluate[seed]*N/M]]) {
		_bestIndex[seed][_nextToEvaluate[seed]*N/M] = _nextToEvaluate[seed];
	}
	// move to next child candidate
	_nextToEvaluate[seed]++;
	
	// if all children evaluated
	if(_nextToEvaluate[seed] == M) {

	    //更新概率模型以及随机采样
	    for(uint i = 0; i < MutationOperatorsNum; i++){
	        int sum = 0;
	        for(uint j = 0; j < N; j++){
	            sum += _individual[seed][_bestIndex[seed][j]][i];
	        }
	        if(sum == N)    sum--;
	        if(sum == 0)    sum++;
	        p[seed][i] = (1 - LEARNING_RATE)*p[seed][i] + LEARNING_RATE * sum / N;

	    }


	    _nextToEvaluate[seed] = 0;

	    for(uint i = 0; i < N; i++){
	        _bestIndex[seed][i] = 5*i;
	    }

	    memset(_Fitness[seed], 0, M);

	}
	
	// if there are individual to evaluate, generate new candidate and return
	if(_nextToEvaluate[seed] < M){
	    _currentRelativeProb[seed] = &(_individual[seed][_nextToEvaluate[seed]][0]);

	    for(uint i = 0; i < MutationOperatorsNum; i++){
	        _currentRelativeProb[seed][i] = (rand_32_double() < p[seed][i]) ? 1 : 0;
	    }
	}
}

/*
 * Get the best parent solution so far as a vector (hardcoded to max mutation operators in AFL)
 * @seed: seed to attribute this to
*/
uint32_t DARWIN_get_parent_repr(uint64_t seed) {
	uint32_t value=0;
	return value;
}

