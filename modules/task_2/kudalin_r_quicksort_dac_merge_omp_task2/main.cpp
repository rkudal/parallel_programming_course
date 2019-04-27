//  Copyright 2019 Kudalin Roman
#define _SCL_SECURE_NO_WARNINGS
#include <omp.h>
#include <iostream>
#include <cstdlib>
#include <algorithm>
#include <iterator>
#include <utility>
#include <ctime>
#include <functional>

enum class SortOrder { DESC, ASC };

int QuickSplit(int* arr, int left, int right,
    int (*comparator)(int arg1, int arg2)) {
    int pivot = arr[(left + right) / 2];
    int i = left, j = right;
    while (i <= j) {
        while (comparator(arr[i], pivot) < 0) {
            i++;
        }
        while (comparator(arr[j], pivot) > 0) {
            j--;
        }
        if (i >= j) {
            break;
        }
        std::swap(arr[i++], arr[j--]);
    }
    return j;
}

void QuickSort(int *arr, int left, int right,
    int (*comparator)(int arg1, int arg2)) {
    if (left < right) {
        int p = QuickSplit(arr, left, right, comparator);
        QuickSort(arr, left, p, comparator);
        QuickSort(arr, p + 1, right, comparator);
    }
}


int* GenerateArray(int size) {
    int* arr = new int[size];
    std::generate(arr, arr + size, []() { return std::rand() % 100; });
    return arr;
}

int* CopyArray(int* src, int size) {
    int* copy = new int[size];
    std::copy(src, src + size, copy);
    return copy;
}

void PrintArray(int* arr, int size) {
    std::for_each(arr, arr + size, [](int elem) { std::cout << elem << " "; });
    std::cout << std::endl;
}

int main(int argc, char** argv) {
    std::srand((unsigned int)std::time(nullptr));
    const int kSize = 10000000;
    int* arr = GenerateArray(kSize);
    int* copied_arr = CopyArray(arr, kSize);
    auto descending = [](int arg1, int arg2) {
        if (arg1 < arg2) {
            return 1;
        }
        if (arg1 > arg2) {
            return -1;
        }
        return 0;
    };
    auto ascending = [](int arg1, int arg2) {
        if (arg1 < arg2) {
            return -1;
        }
        if (arg1 > arg2) {
            return 1;
        }
        return 0;
    };

    int num_threads = omp_get_max_threads();
    int work_per_thread = kSize / num_threads;
    double begin = omp_get_wtime();
    #pragma omp parallel
    {
        #pragma omp for schedule(static) nowait
        for (int i = 0; i < num_threads; ++i) {
            QuickSort(arr, i * work_per_thread, (i + 1) * work_per_thread - 1, ascending);
        }
        #pragma omp single nowait
        {
            if (kSize % num_threads) {
                QuickSort(arr, num_threads * work_per_thread, kSize - 1, ascending);
            }
        }
    }

    int merge_iterations_num = num_threads / 2;
    int merging_parts_size = work_per_thread;
    while (merge_iterations_num > 0) {
        #pragma omp parallel for schedule(static) num_threads(merge_iterations_num)
        for (int i = 0; i < merge_iterations_num; ++i) {
            int* first = arr + 2 * i * merging_parts_size;
            int* middle = first + merging_parts_size;
            int* last = middle + merging_parts_size;
            std::inplace_merge(first, middle, last);
        }
        merging_parts_size *= 2;
        merge_iterations_num /= 2;
    }

    if (kSize % num_threads) {
        std::inplace_merge(arr, arr + num_threads * work_per_thread, arr + kSize);
    }
    double end = omp_get_wtime();
    double parallel_total_time = end - begin;

    std::cout << "Time spent [parallel]: " << parallel_total_time << std::endl;
    std::sort(copied_arr, copied_arr + kSize);
    if (std::equal(arr, arr + kSize, copied_arr)) {
        std::cout << "Parallel version produces the same result as stl version" << std::endl;
    }

    std::random_shuffle(arr, arr + kSize);
    std::random_shuffle(copied_arr, copied_arr + kSize);
    begin = omp_get_wtime();
    QuickSort(arr, 0, kSize - 1, ascending);
    end = omp_get_wtime();
    double linear_total_time = end - begin;
    std::cout << "Time spent [linear]: " << linear_total_time << std::endl;
    std::sort(copied_arr, copied_arr + kSize);
    if (std::equal(arr, arr + kSize, copied_arr)) {
        std::cout << "Linear version produces the same result as stl version" << std::endl;
    }
    std::cout << "Speed up: " << linear_total_time / parallel_total_time << std::endl;

    delete[] arr;
    delete[] copied_arr;
    return 0;
}
