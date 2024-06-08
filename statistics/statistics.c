#include <math.h>
#include <stdlib.h>
#include <float.h>

#include "statistics.h"
#include "../assert/assert.h"
#include "../native/native.h"
#include "../value/value.h"
#include "../vm/vm.h"

static double mean(double* data, int size);
static double weightedMean(double* data, double* weights, int size);
static double geometricMean(double* data, int size);
static double harmonicMean(double* data, int size);
static double median(double* data, int size);
static double medianLow(double* data, int size);
static double medianHigh(double* data, int size);
static double mode(double* data, int size);
static double variance(double* data, int size, int isSample);
static double standardDeviation(double* data, int size, int isSample);
static ObjArray* multiMode(double* data, int size);
static ObjArray* quantiles(double* data, int size, int nQuantiles);

static int compare(const void* a, const void* b) {
  double fa = *(const double*)a;
  double fb = *(const double*)b;
  return (fa > fb) - (fa < fb);
}

NATIVE_FUNCTION(mean) {
  assertArgCount("mean(data)", 1, argCount);
  assertArgIsArray("mean(data)", args, 0);

  ObjArray* array = AS_ARRAY(args[0]);
  double* data = (double*)malloc(array->elements.count * sizeof(double));
  for (int i = 0; i < array->elements.count; i++) {
    assertIsNumber("mean(data)", array->elements.values[i]);
    data[i] = AS_NUMBER(array->elements.values[i]);
  }
  double result = mean(data, array->elements.count);
  free(data);
  RETURN_NUMBER(result);
}

NATIVE_FUNCTION(weightedMean) {
  assertArgCount("weightedMean(data, weights)", 2, argCount);
  assertArgIsArray("weightedMean(data)", args, 0);
  assertArgIsArray("weightedMean(weights)", args, 1);

  ObjArray* dataArray = AS_ARRAY(args[0]);
  ObjArray* weightArray = AS_ARRAY(args[1]);

  int dataSize = dataArray->elements.count;
  int weightSize = weightArray->elements.count;

  if (dataSize != weightSize) {
    runtimeError("Data and weight arrays must have the same length.");
    return false;
  }

  double* data = (double*)malloc(dataSize * sizeof(double));
  double* weights = (double*)malloc(weightSize * sizeof(double));

  for (int i = 0; i < dataSize; i++) {
    assertIsNumber("weightedMean(data)", dataArray->elements.values[i]);
    data[i] = AS_NUMBER(dataArray->elements.values[i]);

    assertIsNumber("weightedMean(weights)", weightArray->elements.values[i]);
    weights[i] = AS_NUMBER(weightArray->elements.values[i]);
  }

  double result = weightedMean(data, weights, dataSize);

  free(data);
  free(weights);

  RETURN_NUMBER(result);
}

NATIVE_FUNCTION(geometricMean) {
  assertArgCount("geometricMean(data)", 1, argCount);
  assertArgIsArray("geometricMean(data)", args, 0);

  ObjArray* array = AS_ARRAY(args[0]);
  double* data = (double*)malloc(array->elements.count * sizeof(double));
  for (int i = 0; i < array->elements.count; i++) {
    assertIsNumber("geometricMean(data)", array->elements.values[i]);
    data[i] = AS_NUMBER(array->elements.values[i]);
  }
  double result = geometricMean(data, array->elements.count);
  free(data);
  RETURN_NUMBER(result);
}

NATIVE_FUNCTION(harmonicMean) {
  assertArgCount("harmonicMean(data)", 1, argCount);
  assertArgIsArray("harmonicMean(data)", args, 0);

  ObjArray* array = AS_ARRAY(args[0]);
  double* data = (double*)malloc(array->elements.count * sizeof(double));
  for (int i = 0; i < array->elements.count; i++) {
    assertIsNumber("harmonicMean(data)", array->elements.values[i]);
    data[i] = AS_NUMBER(array->elements.values[i]);
  }
  double result = harmonicMean(data, array->elements.count);
  free(data);
  RETURN_NUMBER(result);
}

NATIVE_FUNCTION(median) {
  assertArgCount("median(data)", 1, argCount);
  assertArgIsArray("median(data)", args, 0);

  ObjArray* array = AS_ARRAY(args[0]);
  double* data = (double*)malloc(array->elements.count * sizeof(double));
  for (int i = 0; i < array->elements.count; i++) {
    assertIsNumber("median(data)", array->elements.values[i]);
    data[i] = AS_NUMBER(array->elements.values[i]);
  }
  double result = median(data, array->elements.count);
  free(data);
  RETURN_NUMBER(result);
}

NATIVE_FUNCTION(lmedian) {
  assertArgCount("lmedian(data)", 1, argCount);
  assertArgIsArray("lmedian(data)", args, 0);

  ObjArray* array = AS_ARRAY(args[0]);
  double* data = (double*)malloc(array->elements.count * sizeof(double));
  for (int i = 0; i < array->elements.count; i++) {
    assertIsNumber("lmedian(data)", array->elements.values[i]);
    data[i] = AS_NUMBER(array->elements.values[i]);
  }
  double result = medianLow(data, array->elements.count);
  free(data);
  RETURN_NUMBER(result);
}

NATIVE_FUNCTION(hmedian) {
  assertArgCount("hmedian(data)", 1, argCount);
  assertArgIsArray("hmedian(data)", args, 0);

  ObjArray* array = AS_ARRAY(args[0]);
  double* data = (double*)malloc(array->elements.count * sizeof(double));
  for (int i = 0; i < array->elements.count; i++) {
    assertIsNumber("hmedian(data)", array->elements.values[i]);
    data[i] = AS_NUMBER(array->elements.values[i]);
  }
  double result = medianHigh(data, array->elements.count);
  free(data);
  RETURN_NUMBER(result);
}

NATIVE_FUNCTION(mode) {
  assertArgCount("mode(data)", 1, argCount);
  assertArgIsArray("mode(data)", args, 0);

  ObjArray* array = AS_ARRAY(args[0]);
  double* data = (double*)malloc(array->elements.count * sizeof(double));
  for (int i = 0; i < array->elements.count; i++) {
    assertIsNumber("mode(data)", array->elements.values[i]);
    data[i] = AS_NUMBER(array->elements.values[i]);
  }
  double result = mode(data, array->elements.count);
  free(data);
  RETURN_NUMBER(result);
}

NATIVE_FUNCTION(multimode) {
  assertArgCount("multimode(data)", 1, argCount);
  assertArgIsArray("multimode(data)", args, 0);

  ObjArray* array = AS_ARRAY(args[0]);
  double* data = (double*)malloc(array->elements.count * sizeof(double));
  for (int i = 0; i < array->elements.count; i++) {
    assertIsNumber("multimode(data)", array->elements.values[i]);
    data[i] = AS_NUMBER(array->elements.values[i]);
  }
  ObjArray* result = multiMode(data, array->elements.count);
  free(data);
  RETURN_OBJ(result);
}

NATIVE_FUNCTION(quantiles) {
  assertArgCount("quantiles(data, n)", 2, argCount);
  assertArgIsArray("quantiles(data)", args, 0);
  assertArgIsInt("quantiles(n)", args, 1);

  ObjArray* array = AS_ARRAY(args[0]);
  int n = AS_INT(args[1]);

  double* data = (double*)malloc(array->elements.count * sizeof(double));
  for (int i = 0; i < array->elements.count; i++) {
    assertIsNumber("quantiles(data)", array->elements.values[i]);
    data[i] = AS_NUMBER(array->elements.values[i]);
  }

  ObjArray* result = quantiles(data, array->elements.count, n);
  free(data);
  RETURN_OBJ(result);
}

NATIVE_FUNCTION(pstdev) {
  assertArgCount("pstdev(data)", 1, argCount);
  assertArgIsArray("pstdev(data)", args, 0);

  ObjArray* dataArray = AS_ARRAY(args[0]);
  int dataSize = dataArray->elements.count;

  double* data = (double*)malloc(dataSize * sizeof(double));
  for (int i = 0; i < dataSize; i++) {
    assertIsNumber("pstdev(data)", dataArray->elements.values[i]);
    data[i] = AS_NUMBER(dataArray->elements.values[i]);
  }

  double result = standardDeviation(data, dataSize, 0);

  free(data);
  RETURN_NUMBER(result);
}

NATIVE_FUNCTION(stdev) {
  assertArgCount("stdev(data)", 1, argCount);
  assertArgIsArray("stdev(data)", args, 0);

  ObjArray* dataArray = AS_ARRAY(args[0]);
  int dataSize = dataArray->elements.count;

  double* data = (double*)malloc(dataSize * sizeof(double));
  for (int i = 0; i < dataSize; i++) {
    assertIsNumber("stdev(data)", dataArray->elements.values[i]);
    data[i] = AS_NUMBER(dataArray->elements.values[i]);
  }

  double result = standardDeviation(data, dataSize, 1);

  free(data);
  RETURN_NUMBER(result);
}

NATIVE_FUNCTION(pvariance) {
  assertArgCount("pvariance(data)", 1, argCount);
  assertArgIsArray("pvariance(data)", args, 0);

  ObjArray* dataArray = AS_ARRAY(args[0]);
  int dataSize = dataArray->elements.count;

  double* data = (double*)malloc(dataSize * sizeof(double));
  for (int i = 0; i < dataSize; i++) {
    assertIsNumber("pvariance(data)", dataArray->elements.values[i]);
    data[i] = AS_NUMBER(dataArray->elements.values[i]);
  }

  double result = variance(data, dataSize, 0);

  free(data);
  RETURN_NUMBER(result);
}

NATIVE_FUNCTION(variance) {
  assertArgCount("variance(data)", 1, argCount);
  assertArgIsArray("variance(data)", args, 0);

  ObjArray* dataArray = AS_ARRAY(args[0]);
  int dataSize = dataArray->elements.count;

  double* data = (double*)malloc(dataSize * sizeof(double));
  for (int i = 0; i < dataSize; i++) {
    assertIsNumber("variance(data)", dataArray->elements.values[i]);
    data[i] = AS_NUMBER(dataArray->elements.values[i]);
  }

  double result = variance(data, dataSize, 1);

  free(data);
  RETURN_NUMBER(result);
}

static double mean(double* data, int size) {
  double sum = 0.0;
  for (int i = 0; i < size; i++) {
    sum += data[i];
  }
  return sum / size;
}

static double variance(double* data, int size, int isSample) {
  double m = mean(data, size);
  double sum = 0.0;
  for (int i = 0; i < size; i++) {
    double diff = data[i] - m;
    sum += diff * diff;
  }
  return sum / (isSample ? size - 1 : size);
}

static double standardDeviation(double* data, int size, int isSample) {
  return sqrt(variance(data, size, isSample));
}


static double weightedMean(double* data, double* weights, int size) {
  double sum = 0.0;
  double weightSum = 0.0;
  for (int i = 0; i < size; i++) {
    sum += data[i] * weights[i];
    weightSum += weights[i];
  }
  return sum / weightSum;
}

static double geometricMean(double* data, int size) {
  double product = 1.0;
  for (int i = 0; i < size; i++) {
    product *= data[i];
  }
  return pow(product, 1.0 / size);
}

static double harmonicMean(double* data, int size) {
  double sum = 0.0;
  for (int i = 0; i < size; i++) {
    sum += 1.0 / data[i];
  }
  return size / sum;
}

static double median(double* data, int size) {
  qsort(data, size, sizeof(double), compare);
  if (size % 2 == 0) {
    return (data[size / 2 - 1] + data[size / 2]) / 2.0;
  } else {
    return data[size / 2];
  }
}

static double medianLow(double* data, int size) {
  qsort(data, size, sizeof(double), compare);
  return data[(size - 1) / 2];
}

static double medianHigh(double* data, int size) {
  qsort(data, size, sizeof(double), compare);
  return data[size / 2];
}

static double mode(double* data, int size) {
  qsort(data, size, sizeof(double), compare);
  double mode = data[0];
  int maxCount = 1;
  int count = 1;
  for (int i = 1; i < size; i++) {
    if (data[i] == data[i - 1]) {
      count++;
    } else {
      count = 1;
    }
    if (count > maxCount) {
      maxCount = count;
      mode = data[i];
    }
  }
  return mode;
}

static ObjArray* multiMode(double* data, int size) {
  qsort(data, size, sizeof(double), compare);

  ObjArray* modes = newArray();
  push(OBJ_VAL(modes));

  int count = 1;
  int maxCount = 1;
  for (int i = 1; i < size; i++) {
    if (data[i] == data[i - 1]) {
      count++;
    } else {
      if (count > maxCount) {
        maxCount = count;
        modes->elements.count = 0; // Reset the array
        writeValueArray(&modes->elements, NUMBER_VAL(data[i - 1]));
      } else if (count == maxCount) {
        writeValueArray(&modes->elements, NUMBER_VAL(data[i - 1]));
      }
      count = 1;
    }
  }
  if (count > maxCount) {
    modes->elements.count = 0; // Reset the array
    writeValueArray(&modes->elements, NUMBER_VAL(data[size - 1]));
  } else if (count == maxCount) {
    writeValueArray(&modes->elements, NUMBER_VAL(data[size - 1]));
  }

  pop();
  return modes;
}

static ObjArray* quantiles(double* data, int size, int nQuantiles) {
  qsort(data, size, sizeof(double), compare);

  ObjArray* quantileArray = newArray();
  push(OBJ_VAL(quantileArray));

  for (int i = 1; i < nQuantiles; i++) {
    double idx = i * (size - 1) / (double)nQuantiles;
    int intIdx = (int)idx;
    double frac = idx - intIdx;
    double quantile = data[intIdx] * (1.0 - frac) + data[intIdx + 1] * frac;
    writeValueArray(&quantileArray->elements, NUMBER_VAL(quantile));
  }

  pop();
  return quantileArray;
}

void registerStatisticsPackage() {
  ObjNamespace* statisticsNamespace = defineNativeNamespace("statistics", vm.stdNamespace);
  vm.currentNamespace = statisticsNamespace;

  DEF_FUNCTION(mean, 1);
  DEF_FUNCTION(geometricMean, 1);
  DEF_FUNCTION(weightedMean, 2);
  DEF_FUNCTION(harmonicMean, 1);
  DEF_FUNCTION(median, 1);
  DEF_FUNCTION(lmedian, 1);
  DEF_FUNCTION(hmedian, 1);
  DEF_FUNCTION(mode, 1);
  DEF_FUNCTION(multimode, 1);
  DEF_FUNCTION(quantiles, 2);
  DEF_FUNCTION(pstdev, 1);
  DEF_FUNCTION(stdev, 1);
  DEF_FUNCTION(pvariance, 1);
  DEF_FUNCTION(variance, 1);

  vm.currentNamespace = vm.rootNamespace;
}
