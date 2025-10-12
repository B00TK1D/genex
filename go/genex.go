package main

/*
#include "genex.c"
*/
import "C"
import (
	"fmt"
	"unsafe"
)

func callProcess(inputs []string) {
	inputCount := len(inputs)
	if inputCount == 0 {
		fmt.Println("No inputs provided")
		return
	}

	// Initialize memory pool
	var pool C.memory_pool
	C.pool_init(&pool, 1024*1024) // 1MB initial size
	defer C.pool_destroy(&pool)

	// Allocate input_buffers
	var input C.input_buffers
	*(*C.uint)(unsafe.Pointer(&input.count)) = C.uint(inputCount)
	input.values = (*C.bytes)(C.malloc(C.size_t(inputCount) * C.size_t(unsafe.Sizeof(C.bytes{}))))
	defer C.free(unsafe.Pointer(input.values))

	// Convert Go strings to C bytes
	var minLen, maxLen C.ulong = ^C.ulong(0), 0
	valuesSlice := (*[1 << 30]C.bytes)(unsafe.Pointer(input.values))[:inputCount:inputCount]
	cStrings := make([]*C.char, inputCount)

	for i, str := range inputs {
		cStrings[i] = C.CString(str)
		defer C.free(unsafe.Pointer(cStrings[i]))

		valuesSlice[i].len = C.ulong(len(str))
		valuesSlice[i].contents = cStrings[i]

		if valuesSlice[i].len < minLen {
			minLen = valuesSlice[i].len
		}
		if valuesSlice[i].len > maxLen {
			maxLen = valuesSlice[i].len
		}
	}

	input.min_len = minLen
	input.max_len = maxLen

	// Allocate output_buffers
	var output C.output_buffers
	*(*C.uint)(unsafe.Pointer(&output.input_count)) = C.uint(inputCount)
	output.const_count = 0

	// Allocate space for constants (max possible: sum of all input lengths)
	maxConstants := inputCount * 100 // Reasonable upper bound
	output.constants = (*C.bytes)(C.malloc(C.size_t(maxConstants) * C.size_t(unsafe.Sizeof(C.bytes{}))))
	defer C.free(unsafe.Pointer(output.constants))

	// Allocate space for variables (2D array)
	output.variables = (**C.bytes)(C.malloc(C.size_t(maxConstants) * C.size_t(unsafe.Sizeof(uintptr(0)))))
	defer C.free(unsafe.Pointer(output.variables))

	varsSlice := (*[1 << 30]*C.bytes)(unsafe.Pointer(output.variables))[:maxConstants:maxConstants]
	for i := 0; i < maxConstants; i++ {
		varsSlice[i] = (*C.bytes)(C.malloc(C.size_t(inputCount) * C.size_t(unsafe.Sizeof(C.bytes{}))))
		defer C.free(unsafe.Pointer(varsSlice[i]))
	}

	// Call process
	C.process(&input, &output, &pool)

	// Print output using C function
	fmt.Printf("Input: %v\nOutput: ", inputs)
	C.print_output(&output)
}
