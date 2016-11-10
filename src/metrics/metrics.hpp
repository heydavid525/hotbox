#pragma once
#include "metrics/proto/metrics.pb.h"
#include <vector>
#include <iostream>

namespace hotbox {
  // TransStats is used by mtt to sample the overall stats during
  // transformation
  // collects by first thread in each stage
  // and by some of the tasks
  struct TransStats {
    int ntransforms;
    // micro
    // NOTE: we are using hotbox::Timer (steady_clock) for measuring time
    // we can also use std::clock() to measure cycles
    float t_input; // time to read the input dataset
    unsigned long n_input; // number of times t_input is sampled

    std::vector<float> t_rcache; // time to read in the cache
    unsigned long n_rcache;

    std::vector<float> t_transform; // time to execute transform
    std::vector<unsigned long long> n_generated_value; // generated values for estimating size
    unsigned long n_transform;
    
    std::vector<float> t_wcache; // time to write out to cache
    unsigned long n_wcache;

    //float t_client; // time for client to consume
    //unsigned long n_client;

    //// macro
    //float t_task; // end to end time to complete a task
    //float t_pure_task; // end to end time to complete a task without reading input dataset
    //unsigned long n_task;

    inline void init(int n) {
      t_input = 0;
      n_input = 0;
      n_rcache = 0;
      n_transform = 0;
      n_wcache = 0;
      ntransforms = n;
      t_transform.resize(n, 0.f);
      n_generated_value.resize(n, 0);
      t_rcache.resize(n, 0.f);
      t_wcache.resize(n, 0.f);
    }
    inline void add_input(float t) {
      t_input += t;
      n_input++;
    }
    inline void add_rcache() {
      n_rcache++;
    }
    inline void add_rcache(int i, float t) {
      t_rcache[i] += t;
    }
    inline void add_transform() {
      n_transform++;
    }
    inline void add_transform(int i, float t, unsigned long long nvals) {
      t_transform[i] += t;
      n_generated_value[i] += nvals;
    }
    inline void add_wcache() {
      n_wcache++;
    }
    inline void add_wcache(int i, float t) {
      t_wcache[i] += t;
    }
    //void add_task(float full, float pure) {
      //t_task += full;
      //t_pure_task += pure;
      //n_task ++;
    //}
    // calculate cost and gain for transformation
    // all in seconds
    // time to write out, time to read in, time to stage in, compression,
    // decompression
    float cost(int t) {
      float io_const_cost = 0.05;
      // estimate size in mb then 100mb/s
      float cache_in_cost = io_const_cost + (float) n_generated_value[t]/n_transform*4.0/1024/1024/100;
      // mem cpy
      float cost = cache_in_cost + io_const_cost + (float) n_generated_value[t]/n_transform*4.0/1024/1024/1024/10;
      return cost;
    }
    float gain(int t) {
      float gain = t_transform[t]/n_transform;
      return gain;
    }
    // decide whether to cache for given transformation
    bool decision(int t) {
      return (gain(t) > cost(t));
    }
    inline void print() {
      if (n_input == 0) {
        std::cout<<"time to read the input dataset: not sampled\n";
      } else {
        std::cout<<"time to read the input dataset: "<<t_input/n_input<<std::endl;
      }
      if (n_rcache == 0) {
        std::cout<<"time to read the cache [tid:time]: not sampled";
      } else {
        std::cout<<"time to read the cache [tid:time]: ";
        for (int i = 0; i < t_rcache.size(); ++i) {
          std::cout<<"["<<i<<":"<<t_rcache[i]/n_rcache<<"] ";
        }
      }
      if (n_transform == 0) {
        std::cout<<"\ntime to execute transform / cache in [tid:time:vals]: not sampled";
      } else {
        std::cout<<"\ntime to execute transform / cache in [tid:time:vals]: ";
        for (int i = 0; i < t_transform.size(); ++i) {
          std::cout<<"["<<i<<":"<<t_transform[i]/n_transform<<":"<<n_generated_value[i]/n_transform<<"] ";
        }
      }
      if (n_wcache == 0) {
        std::cout<<"\ntime to write the cache [tid:time]: not sampled";
      } else {
        std::cout<<"\ntime to write the cache [tid:time]: ";
        for (int i = 0; i < t_wcache.size(); ++i) {
          std::cout<<"["<<i<<":"<<t_wcache[i]/n_wcache<<"] ";
        }
      }
      std::cout<<"\ndecision [tid:cost:gain:decition]: ";
      std::cout.precision(3);
      for (int i = 0; i < t_transform.size(); ++i) {
          std::cout<<"["<<i<<":"<<cost(i)<<":"<<gain(i)<<":"<<decision(i)<<"] ";
      }
      std::cout<<"\n";
    }
  };
}

// latency, throughput, per session/atom
