package main

/*
#cgo CFLAGS: -O3
#cgo LDFLAGS: -L${SRCDIR}
#include "genex.c"
#include <stdlib.h>
*/
import "C"
import (
	"fmt"
	"unsafe"
)

type Bytes struct {
	Len      uint64
	Contents []byte
}

type InputBuffers struct {
	Count  uint32
	MinLen uint64
	MaxLen uint64
	Values []*Bytes
}

type OutputBuffers struct {
	InputCount uint32
	ConstCount uint64
	Constants  []*Bytes
	Variables  []*Bytes
}

// exportedProcess wraps C.process
func Process(input *InputBuffers) []*Bytes {
	// --- Allocate C input struct
	cInput := C.malloc(C.size_t(unsafe.Sizeof(C.input_buffers{})))
	defer C.free(cInput)
	inputC := (*C.input_buffers)(cInput)

	inputC.count = C.uint(input.Count)
	inputC.min_len = C.ulong(input.MinLen)
	inputC.max_len = C.ulong(input.MaxLen)

	// Allocate the bytes array
	cValues := C.malloc(C.size_t(input.Count) * C.size_t(unsafe.Sizeof(C.bytes{})))
	inputC.values = (*C.bytes)(cValues)

	// Copy Go []byte → C bytes[]
	for i := uint32(0); i < input.Count; i++ {
		goB := input.Values[i]
		cElem := (*C.bytes)(unsafe.Pointer(uintptr(cValues) + uintptr(i)*unsafe.Sizeof(C.bytes{})))
		cElem.len = C.ulong(len(goB.Contents))
		cElem.contents = (*C.char)(C.CBytes(goB.Contents))
	}

	// --- Prepare C output struct
	cOutput := C.malloc(C.size_t(unsafe.Sizeof(C.output_buffers{})))
	defer C.free(cOutput)
	outputC := (*C.output_buffers)(cOutput)

	outputC.input_count = C.uint(input.Count)
	outputC.const_count = 0
	outputC.constants = nil

	// Allocate space for variable pointers
	cVars := C.malloc(C.size_t(input.Count) * C.size_t(unsafe.Sizeof(uintptr(0))))
	outputC.variables = (**C.bytes)(cVars)

	// --- Call the C function
	C.process(inputC, outputC, nil)

	// --- Extract results
	result := make([]*Bytes, input.Count)
	for i := uint32(0); i < input.Count; i++ {
		// outputC.variables[i] points to a C.bytes*
		cVarPtr := *(**C.bytes)(unsafe.Pointer(uintptr(cVars) + uintptr(i)*unsafe.Sizeof(uintptr(0))))
		if cVarPtr == nil {
			continue
		}

		data := C.GoBytes(unsafe.Pointer(cVarPtr.contents), C.int(cVarPtr.len))
		result[i] = &Bytes{
			Len:      uint64(len(data)),
			Contents: data,
		}

		// free the C allocations from process()
		C.free(unsafe.Pointer(cVarPtr.contents))
		C.free(unsafe.Pointer(cVarPtr))
	}

	// free input bytes
	for i := uint32(0); i < input.Count; i++ {
		cElem := (*C.bytes)(unsafe.Pointer(uintptr(cValues) + uintptr(i)*unsafe.Sizeof(C.bytes{})))
		C.free(unsafe.Pointer(cElem.contents))
	}

	C.free(cValues)
	C.free(cVars)

	return result
}

func main() {
	inputs := []*Bytes{
		{Contents: []byte("TEST: abcd<<")},
		{Contents: []byte("TEST: efgh<<")},
	}

	inputBuf := &InputBuffers{
		Count:  uint32(len(inputs)),
		Values: inputs,
	}

	results := Process(inputBuf)
	for _, r := range results {
		fmt.Println(string(r.Contents))
	}
}
