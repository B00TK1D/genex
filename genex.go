package genex

/*
#include "genex.c"
*/
import "C"
import (
	"unsafe"
)

type Field struct {
	Constant  []byte
	Variables [][]byte
}

type Protocol struct {
	Field []Field
}

func Genex(inputs [][]byte) Protocol {
	inputCount := len(inputs)
	result := Protocol{}
	if inputCount == 0 {
		return result
	}

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
		cStrings[i] = C.CString(string(str))
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

	// Initialize memory pool
	var pool C.memory_pool
	initialPoolSize := C.ulong(inputCount) * maxLen * C.ulong(unsafe.Sizeof(C.ulong(0))) * 100
	C.pool_init(&pool, initialPoolSize)
	defer C.pool_destroy(&pool)

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

	// Generate Go structures from C output
	constantsSlice := (*[1 << 30]C.bytes)(unsafe.Pointer(output.constants))[:output.const_count:output.const_count]
	for i := 0; i < int(output.const_count); i++ {
		field := Field{}
		field.Constant = C.GoBytes(unsafe.Pointer(constantsSlice[i].contents), C.int(constantsSlice[i].len))

		varsForField := (*[1 << 30]C.bytes)(unsafe.Pointer(varsSlice[i]))[:inputCount:inputCount]
		for j := 0; j < inputCount; j++ {
			varBytes := C.GoBytes(unsafe.Pointer(varsForField[j].contents), C.int(varsForField[j].len))
			field.Variables = append(field.Variables, varBytes)
		}

		result.Field = append(result.Field, field)
	}
	return result
}
