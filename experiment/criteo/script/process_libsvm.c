#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
// change 28 to 29, because day 21 blocked when using 28
#define HASH_CAPACITY (1 << 29)

typedef struct HashTable {
  uint32_t capacity;
  uint32_t size;
  uint32_t *keys;
  uint32_t *values;
  uint8_t *used;
} HashTable;

HashTable *hash_table_create() {
  HashTable *ret = (HashTable*) malloc(sizeof(HashTable));
  ret->capacity = HASH_CAPACITY;
  ret->size = 0;
  ret->keys = (uint32_t*) malloc(ret->capacity * sizeof(uint32_t));
  ret->values = (uint32_t*) malloc(ret->capacity * sizeof(uint32_t));
  ret->used = (uint8_t*) malloc(ret->capacity * sizeof(uint8_t));
  memset(ret->used, 0, ret->capacity * sizeof(uint8_t));
  return ret;
}

HashTable *hash_table_insert(HashTable *hash_table, uint32_t key, uint32_t value) {
  uint32_t index = key % hash_table->capacity;
  while(1) {
    if (!hash_table->used[index]) {
      hash_table->used[index] = 1;
      hash_table->keys[index] = key;
      hash_table->values[index] = value;
      hash_table->size ++;
      break;
    } else if (hash_table->keys[index] == key) {
      break;
    } else {
      index = (index + 1) % hash_table->capacity;
    }
  }
  return hash_table;
}

int32_t hash_table_contains(HashTable *hash_table, uint32_t key) {
  uint32_t index = key % hash_table->capacity;
  while(1) {
    if (!hash_table->used[index]) {
      return 0;
    } else if (hash_table->keys[index] == key) {
      return 1;
    } else {
      index = (index + 1) % hash_table->capacity;
    }
  }
}

uint32_t hash_table_get(HashTable *hash_table, uint32_t key) {
  uint32_t index = key % hash_table->capacity;
  while(1) {
    assert(hash_table->used[index]);
    if (hash_table->keys[index] == key) {
      return hash_table->values[index];
    } else {
      index = (index + 1) % hash_table->capacity;
    }
  }
}

typedef struct Sample {
  uint32_t label;
  uint32_t nnz;
  uint32_t feature_ids[39];
  uint32_t features[39];
  uint32_t num_counts;
} Sample;

int32_t get_next_int(FILE *fp, uint32_t *ret) {
  *ret = 0;
  int32_t exist = 0;
  int32_t neg = 0;
  while(1) {
    int32_t c = getc(fp);
    if (c == '-') {
      neg = 1;
      exist = 1;
    } else if (c >= '0' && c <= '9') {
      *ret = *ret * 10 + (c - '0');
      exist = 1;
    } else {
      break;
    }
  }
  if (neg) {
    *ret = -(*ret);
  }
  return exist;
}

int32_t get_next_hash(FILE *fp, uint32_t *ret) {
  *ret = 0;
  int32_t exist = 0;
  while(1) {
    int32_t c = getc(fp);
    if (c >= '0' && c <= '9') {
      *ret = *ret * 16 + (c - '0');
      exist = 1;
    } else if (c >= 'a' && c <= 'f') {
      *ret = *ret * 16 + (c - 'a' + 10); 
      exist = 1;
    } else {
      break;
    }
  }
  return exist;
}


int32_t get_next_sample(FILE *fp, Sample *sample) {
  int ret = get_next_int(fp, &sample->label);
  if (feof(fp)) {
    return 0;
  }
  assert(ret);
  assert(sample->label == 0 || sample->label == 1);
  int i;
  int nnz = 0;
  for (i = 0; i < 13; i ++) {
    ret = get_next_int(fp, &sample->features[nnz]);
    if (ret) {
      sample->feature_ids[nnz] = i;
      nnz ++;
    }
  }
  sample->num_counts = nnz;
  for (i = 13; i < 39; i ++) {
    ret = get_next_hash(fp, &sample->features[nnz]);
    if (ret) {
      sample->feature_ids[nnz] = i;
      nnz ++;
    }
  }
  sample->nnz = nnz;
  return 1;
}


typedef struct Dictionary {
  HashTable *hash_tables[39];
  uint32_t next_index;
} Dictionary;


HashTable *hash_table_load(FILE *fp) {
  HashTable *ret = hash_table_create();
  uint32_t key, val;
  while (fscanf(fp, "%x %d", &key, &val) != EOF) {
    hash_table_insert(ret, key, val);
  }
  return ret;
}


void hash_table_save(HashTable *hash_table, FILE *fp) {
  int i;
  for (i = 0; i < hash_table->capacity; i ++) {
    if (hash_table->used[i]) {
      fprintf(fp, "%x %u\n", hash_table->keys[i], hash_table->values[i]);
    }
  }
}


Dictionary *load_dict(const char *dict_dir) {
  Dictionary *dict = (Dictionary*) malloc(sizeof(Dictionary));
  dict->next_index = 13;
  int i;
  char path[strlen(dict_dir) + 64];
  for (i = 13; i < 39; i ++) {
    //printf("%d hash table created\n", i);
    sprintf(path, "%s/dict_cat_%d.txt", dict_dir, i);
    FILE *fp = fopen(path, "r");
    dict->hash_tables[i] = hash_table_load(fp);
    dict->next_index += dict->hash_tables[i]->size;
  }
  return dict;
}


void save_dict(const char *dict_dir, Dictionary *dict) {
  int i;
  char path[strlen(dict_dir) + 64];
  for (i = 13; i < 39; i ++) {
    sprintf(path, "%s/dict_cat_%d.txt", dict_dir, i);
    FILE *fp = fopen(path, "w");
    if (fp == 0) {
      printf("failed to open %s\n", path);
      abort();
    }
    hash_table_save(dict->hash_tables[i], fp);
  }
}


uint32_t get_hash_feature_id(uint32_t feature_id, uint32_t hash, Dictionary *dict) {
  HashTable *hash_table = dict->hash_tables[feature_id];
  if (hash_table_contains(hash_table, hash)) {
    return hash_table_get(hash_table, hash);
  } else {
    hash_table_insert(hash_table, hash, dict->next_index);
    dict->next_index ++;
    return hash_table_get(hash_table, hash);
  }
}


void convert_sample(Sample* sample, Dictionary *dict) {
  int i;
  for (i = sample->num_counts; i < sample->nnz; i ++) {
    sample->feature_ids[i] =
        get_hash_feature_id(sample->feature_ids[i], sample->features[i], dict);
    sample->features[i] = 1;
  }
  int j, t;
  for (i = sample->num_counts + 1; i < sample->nnz; i++) {
    t = sample->feature_ids[i];
    for (j = i; j > sample->num_counts && t < sample->feature_ids[j - 1]; j --) {
        sample->feature_ids[j] = sample->feature_ids[j - 1];
    }
    sample->feature_ids[j] = t;
  }
}


void output_binary_sample(FILE *fp, Sample *sample) {
  fwrite(&sample->label, sizeof(uint32_t), 1, fp);
  fwrite(&sample->nnz, sizeof(uint32_t), 1, fp);
  fwrite(&sample->feature_ids, sizeof(uint32_t), sample->nnz, fp);
  fwrite(&sample->features, sizeof(uint32_t), sample->nnz, fp);
}

void output_libsvm_sample(FILE *fp, Sample *sample) {
  fprintf(fp, "%d", sample->label);
  int i = 0;
  for (i = 0; i < sample->nnz; i++) {
    fprintf(fp, " %d:%d", sample->feature_ids[i], sample->features[i]);
  }
  fprintf(fp, "\n");
}

Dictionary *create_empty_dict() {
  Dictionary *dict = (Dictionary*) malloc(sizeof(Dictionary));
  dict->next_index = 13;
  int i;
  for (i = 13; i < 39; i ++) {
    dict->hash_tables[i] = hash_table_create();
    dict->next_index += dict->hash_tables[i]->size;
  }
  return dict;
}

int main(int argc, char *argv[]) {
  char *data_path = argv[1];
  char *dict_path = argv[2];
  char *output_path = argv[3];
  
  FILE *data_fp = fopen(data_path, "r");
#ifndef DAY0
  Dictionary *dict = load_dict(dict_path);
  // use this when converting day 0
#else
  Dictionary *dict = create_empty_dict();
#endif
  char path[strlen(output_path) + 64];
  sprintf(path, "%s/data.libsvm.0", output_path);
  FILE *output_fp = fopen(path, "w");
  Sample sample;
  int next_file_id = 1;
  int samples = 0;
  while (get_next_sample(data_fp, &sample)) {
    convert_sample(&sample, dict);
    //output_binary_sample(output_fp, &sample);
    output_libsvm_sample(output_fp, &sample);
    if (ftell(output_fp) >= (1 << 30)) {
      fclose(output_fp);
      sprintf(path, "%s/data.libsvm.%d", output_path, next_file_id);
      //sprintf(path, "%s/data.bin.%d", output_path, next_file_id);
      output_fp = fopen(path, "w");
      //output_fp = fopen(path, "wb");
      next_file_id ++;
    }
    samples ++;
    if (samples % 100000 == 0) {
      printf("Samples processed: %d\n", samples);
      printf("Number of features: %d\n", dict->next_index);
      // break;
    }
  }
  fclose(output_fp);
  
  sprintf(path, "%s/dict", output_path);
  save_dict(path, dict);
}
