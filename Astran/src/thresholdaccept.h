#ifndef THRESHOLD_ACCEPT_H
#define THRESHOLD_ACCEPT_H

#define ever (;;)

#include <iostream>
#include <vector>
#include <math.h>
#include <ctime>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


using namespace std;

inline double GetNextThreshold(double threshold, double accept_ratio, double dp) {
    
    if (accept_ratio > 0.98)
        return double(threshold)*0.6;
    
    if (accept_ratio > 0.2)
        return double(threshold)*0.96;
    
    if (threshold == 0 || accept_ratio == 0 || dp == 0)
        return 0;
    
    return double(threshold)*0.98;
}

inline bool TA_Accept(double delta, double threshold) {
    if (delta <= 0)
        return true;
    
    return delta<=threshold;
}

template<class toOptimize>
int FindInitialThreshold(toOptimize& obj) {
    
    //  cout << " <FindInitialThreshold";
    
    int temp_max=0xFFFFFFF;
    int temp_min=0;
    
    int temp;
    double avg_delta;
    double delta;
    int cont_delta;
    
    toOptimize obj_copy = obj;
    
    //  cout << "[Cost";
    double ini_cost = obj.GetCost();
    double cost=ini_cost;
    
    //  cout << "]";
    
    //  LOG("Running FindInitialThreshold...\n");
    
    
    do {
        
        //    cerr << ".";
        
        obj_copy = obj;
        temp = (temp_max-temp_min)/2 + temp_min;
        cont_delta = 0;
        
        //   LOG("  trying threshold %i [ %i ; %i ] ", temp, temp_min, temp_max);
        
        for (int r=0; r<5000; r++) {
            obj_copy.Perturbation();
            delta = obj_copy.GetCost()-cost;
            
            if (TA_Accept(delta,temp)) {
                avg_delta += delta;
                cost += delta;
                cont_delta++;
            }
            else
                obj_copy.UndoPerturbation();
        }
        
        avg_delta/=cont_delta;
        //   LOG(" avg_delta = %.6f ini cost = %.6f norm avg_delta = %.6f\n",avg_delta, ini_cost, avg_delta/ini_cost);
        
        if (avg_delta <0)
            temp_min = (temp_max-temp_min)/2 + temp_min;
        else
            temp_max = (temp_max-temp_min)/2 + temp_min;
        
    } while (!((avg_delta/ini_cost < 0.000000001 && avg_delta/ini_cost>-0.000000001) || ((temp_max - temp_min) <= 1)));
    
    //  cout << ">";
    
    return max(temp,0);
}


//Quality factor is generally between (0;1] but can be more than 1. Controls the quality vs speed trade-off
template <class toOptimize>
toOptimize ThresholdAccept(toOptimize& initial_solution, double quality_factor, bool greedy=false, bool print_progress=true, bool high_threshold=false){
    double threshold = (greedy?0:(high_threshold?99999:FindInitialThreshold<toOptimize>(initial_solution)));
    //int temperature = 99999999;
    //int temperature = 0;
    double cost = initial_solution.GetCost();
    double delta = 0;
    double avg_delta;
    double accept_ratio;
    double average_cost;
    int cont_accepts;
    int num_repetitions = int(initial_solution.GetProblemSize()*quality_factor);
    
    if (cost == 0 )
        return initial_solution;
    
    if (num_repetitions < 1)
        num_repetitions = 1;
    
    double var_1,var_2;
    
    double previous_cost = cost;
    int cont_same_cost = 0;
    double cost_ratio;
    
    toOptimize solution = initial_solution;
    
    if (print_progress)
        cout << " ( initial cost = " << cost << " num reps = " << num_repetitions << " problem size = " << initial_solution.GetProblemSize() << " ) ";
    
    //  LOG("Running Simulated Annealing... Initial cost =  %..3fi\n",cost);
    int iter = 0;
    
    int time_of_iteration_ini = clock();
    
    for ever {
        iter++;
        
        cont_accepts = 0;
        average_cost = 0;
        var_1 = 0;
        var_2 = 0;
        avg_delta = 0;
        
        
        
        for (int i=0; i<num_repetitions; i++) {
            
            //      cerr << "0";
            
            solution.Perturbation();
            delta = solution.GetCost()-cost;
            
            //      cerr << "1";
            if (TA_Accept(delta,threshold)) {
                cont_accepts++;
                cost += delta;
                avg_delta += delta;
            }
            else {
                //	cerr << "2";
                solution.UndoPerturbation();
                //	cerr << "3";
            }
            
            var_1 += cost*cost/num_repetitions;
            var_2 += cost/num_repetitions;
            
            average_cost = (i*average_cost + cost) / (i+1);
        }
        
        double variancia = var_1-var_2*var_2;
        double desvio_padrao;
        
        double time_of_iteration = double(clock() - time_of_iteration_ini)/double(CLOCKS_PER_SEC);
        
        if (time_of_iteration > 17500) break;
        
        
        if (cont_accepts > 1)
            desvio_padrao = sqrt(variancia);
        else
            desvio_padrao = 0;
        
        if (cont_accepts>0)
            avg_delta /= cont_accepts;
        else
            avg_delta = 0;
        
        if (cont_accepts>0)
            accept_ratio = double(cont_accepts)/double(num_repetitions);
        else
            accept_ratio = 0.0;
        
        cost_ratio = cost/previous_cost;
        if ((cost_ratio >= 0.9999 && cost_ratio <= 1 && (accept_ratio < 0.8 || cost == previous_cost))) {
            cont_same_cost++;
        }
        else {
            cont_same_cost = 0;
            previous_cost = cost;
        }
        
        if (cont_same_cost > 9)
            break;
        
        double bla = solution.GetCost();
        
        if (bla != cost) {
            cout << "INCONSISTENCY ERROR " << bla << " <> " << cost << "\n";
        }
        
        if (print_progress)
            printf(" Iteration %i; cost = %.3f (%.3f); p_cost = %.3f; accept = %.3f; avg_delta = %.3f; dp = %.3f; thres = %.3f; cost_ratio = %.3f; [%i]\n",
                   iter,cost,bla,previous_cost,accept_ratio,avg_delta,desvio_padrao,threshold,cost_ratio,cont_same_cost);
        
        if (print_progress)
            cerr << ".";
        
        threshold = GetNextThreshold(threshold,accept_ratio, desvio_padrao);
    }
    
    if (print_progress)
        cout << " final cost = " << cost << " ";
    
    return solution;
}




#endif
