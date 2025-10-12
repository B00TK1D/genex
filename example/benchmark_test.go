package main

import (
	"fmt"
	"math/rand"
	"os"
	"path/filepath"
	"testing"

	genex "github.com/B00TK1D/genex"
)

var testInputsCache [][]byte

// loadTestInputs loads all test input files from the test_inputs directory
func loadTestInputs() ([][]byte, error) {
	if testInputsCache != nil {
		return testInputsCache, nil
	}

	inputDir := "test_inputs"
	entries, err := os.ReadDir(inputDir)
	if err != nil {
		return nil, fmt.Errorf("failed to read test_inputs directory: %w", err)
	}

	var inputs [][]byte
	for _, entry := range entries {
		if entry.IsDir() {
			continue
		}

		filePath := filepath.Join(inputDir, entry.Name())
		data, err := os.ReadFile(filePath)
		if err != nil {
			return nil, fmt.Errorf("failed to read file %s: %w", filePath, err)
		}

		inputs = append(inputs, data)
	}

	testInputsCache = inputs
	return inputs, nil
}

// selectRandomInputs selects n random inputs from the available test inputs
func selectRandomInputs(allInputs [][]byte, n int, rng *rand.Rand) [][]byte {
	if n > len(allInputs) {
		n = len(allInputs)
	}

	selected := make([][]byte, n)
	indices := rng.Perm(len(allInputs))[:n]

	for i, idx := range indices {
		selected[i] = allInputs[idx]
	}

	return selected
}

// generateRandomString generates a random string of the specified length
func generateRandomString(length int, rng *rand.Rand) []byte {
	const charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 .,;:!?-_/\\@#$%^&*()[]{}|<>"
	result := make([]byte, length)
	for i := range result {
		result[i] = charset[rng.Intn(len(charset))]
	}
	return result
}

// generateRandomInputs generates n random strings of the specified length
func generateRandomInputs(count, length int, rng *rand.Rand) [][]byte {
	inputs := make([][]byte, count)
	for i := range inputs {
		inputs[i] = generateRandomString(length, rng)
	}
	return inputs
}

// BenchmarkGenex2 benchmarks Genex with 2 random inputs
func BenchmarkGenex2(b *testing.B) {
	inputs, err := loadTestInputs()
	if err != nil {
		b.Fatal(err)
	}

	if len(inputs) < 2 {
		b.Skip("Not enough test inputs")
	}

	rng := rand.New(rand.NewSource(42))

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		testInputs := selectRandomInputs(inputs, 2, rng)
		_ = genex.Genex(testInputs)
	}
}

// BenchmarkGenex3 benchmarks Genex with 3 random inputs
func BenchmarkGenex3(b *testing.B) {
	inputs, err := loadTestInputs()
	if err != nil {
		b.Fatal(err)
	}

	if len(inputs) < 3 {
		b.Skip("Not enough test inputs")
	}

	rng := rand.New(rand.NewSource(42))

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		testInputs := selectRandomInputs(inputs, 3, rng)
		_ = genex.Genex(testInputs)
	}
}

// BenchmarkGenex5 benchmarks Genex with 5 random inputs
func BenchmarkGenex5(b *testing.B) {
	inputs, err := loadTestInputs()
	if err != nil {
		b.Fatal(err)
	}

	if len(inputs) < 5 {
		b.Skip("Not enough test inputs")
	}

	rng := rand.New(rand.NewSource(42))

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		testInputs := selectRandomInputs(inputs, 5, rng)
		_ = genex.Genex(testInputs)
	}
}

// BenchmarkGenex10 benchmarks Genex with 10 random inputs
func BenchmarkGenex10(b *testing.B) {
	inputs, err := loadTestInputs()
	if err != nil {
		b.Fatal(err)
	}

	if len(inputs) < 10 {
		b.Skip("Not enough test inputs")
	}

	rng := rand.New(rand.NewSource(42))

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		testInputs := selectRandomInputs(inputs, 10, rng)
		_ = genex.Genex(testInputs)
	}
}

// BenchmarkGenex20 benchmarks Genex with 20 random inputs
func BenchmarkGenex20(b *testing.B) {
	inputs, err := loadTestInputs()
	if err != nil {
		b.Fatal(err)
	}

	if len(inputs) < 20 {
		b.Skip("Not enough test inputs")
	}

	rng := rand.New(rand.NewSource(42))

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		testInputs := selectRandomInputs(inputs, 20, rng)
		_ = genex.Genex(testInputs)
	}
}

// BenchmarkGenex50 benchmarks Genex with 50 random inputs
func BenchmarkGenex50(b *testing.B) {
	inputs, err := loadTestInputs()
	if err != nil {
		b.Fatal(err)
	}

	if len(inputs) < 50 {
		b.Skip("Not enough test inputs")
	}

	rng := rand.New(rand.NewSource(42))

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		testInputs := selectRandomInputs(inputs, 50, rng)
		_ = genex.Genex(testInputs)
	}
}

// BenchmarkGenex100 benchmarks Genex with 100 random inputs
func BenchmarkGenex100(b *testing.B) {
	inputs, err := loadTestInputs()
	if err != nil {
		b.Fatal(err)
	}

	if len(inputs) < 100 {
		b.Skip("Not enough test inputs")
	}

	rng := rand.New(rand.NewSource(42))

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		testInputs := selectRandomInputs(inputs, 100, rng)
		_ = genex.Genex(testInputs)
	}
}

// BenchmarkGenexAllInputs benchmarks Genex with all available test inputs
func BenchmarkGenexAllInputs(b *testing.B) {
	inputs, err := loadTestInputs()
	if err != nil {
		b.Fatal(err)
	}

	if len(inputs) == 0 {
		b.Skip("No test inputs available")
	}

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		_ = genex.Genex(inputs)
	}
}

// Random string benchmarks with varying sizes and input counts

// 10-byte strings
func BenchmarkGenexRandom_2x10(b *testing.B) {
	rng := rand.New(rand.NewSource(42))
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		inputs := generateRandomInputs(2, 10, rng)
		_ = genex.Genex(inputs)
	}
}

func BenchmarkGenexRandom_10x10(b *testing.B) {
	rng := rand.New(rand.NewSource(42))
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		inputs := generateRandomInputs(10, 10, rng)
		_ = genex.Genex(inputs)
	}
}

func BenchmarkGenexRandom_100x10(b *testing.B) {
	rng := rand.New(rand.NewSource(42))
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		inputs := generateRandomInputs(100, 10, rng)
		_ = genex.Genex(inputs)
	}
}

func BenchmarkGenexRandom_1000x10(b *testing.B) {
	rng := rand.New(rand.NewSource(42))
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		inputs := generateRandomInputs(1000, 10, rng)
		_ = genex.Genex(inputs)
	}
}

// 100-byte strings
func BenchmarkGenexRandom_2x100(b *testing.B) {
	rng := rand.New(rand.NewSource(42))
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		inputs := generateRandomInputs(2, 100, rng)
		_ = genex.Genex(inputs)
	}
}

func BenchmarkGenexRandom_10x100(b *testing.B) {
	rng := rand.New(rand.NewSource(42))
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		inputs := generateRandomInputs(10, 100, rng)
		_ = genex.Genex(inputs)
	}
}

func BenchmarkGenexRandom_100x100(b *testing.B) {
	rng := rand.New(rand.NewSource(42))
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		inputs := generateRandomInputs(100, 100, rng)
		_ = genex.Genex(inputs)
	}
}

func BenchmarkGenexRandom_1000x100(b *testing.B) {
	rng := rand.New(rand.NewSource(42))
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		inputs := generateRandomInputs(1000, 100, rng)
		_ = genex.Genex(inputs)
	}
}

// 1000-byte strings
func BenchmarkGenexRandom_2x1000(b *testing.B) {
	rng := rand.New(rand.NewSource(42))
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		inputs := generateRandomInputs(2, 1000, rng)
		_ = genex.Genex(inputs)
	}
}

func BenchmarkGenexRandom_10x1000(b *testing.B) {
	rng := rand.New(rand.NewSource(42))
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		inputs := generateRandomInputs(10, 1000, rng)
		_ = genex.Genex(inputs)
	}
}

func BenchmarkGenexRandom_100x1000(b *testing.B) {
	rng := rand.New(rand.NewSource(42))
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		inputs := generateRandomInputs(100, 1000, rng)
		_ = genex.Genex(inputs)
	}
}

func BenchmarkGenexRandom_500x1000(b *testing.B) {
	rng := rand.New(rand.NewSource(42))
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		inputs := generateRandomInputs(500, 1000, rng)
		_ = genex.Genex(inputs)
	}
}

func BenchmarkGenexRandom_1000x1000(b *testing.B) {
	rng := rand.New(rand.NewSource(42))
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		inputs := generateRandomInputs(1000, 1000, rng)
		_ = genex.Genex(inputs)
	}
}

// 10000-byte strings
func BenchmarkGenexRandom_2x10000(b *testing.B) {
	rng := rand.New(rand.NewSource(42))
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		inputs := generateRandomInputs(2, 10000, rng)
		_ = genex.Genex(inputs)
	}
}

func BenchmarkGenexRandom_10x10000(b *testing.B) {
	rng := rand.New(rand.NewSource(42))
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		inputs := generateRandomInputs(10, 10000, rng)
		_ = genex.Genex(inputs)
	}
}

func BenchmarkGenexRandom_50x10000(b *testing.B) {
	rng := rand.New(rand.NewSource(42))
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		inputs := generateRandomInputs(50, 10000, rng)
		_ = genex.Genex(inputs)
	}
}

func BenchmarkGenexRandom_100x10000(b *testing.B) {
	rng := rand.New(rand.NewSource(42))
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		inputs := generateRandomInputs(100, 10000, rng)
		_ = genex.Genex(inputs)
	}
}

func BenchmarkGenexRandom_500x10000(b *testing.B) {
	rng := rand.New(rand.NewSource(42))
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		inputs := generateRandomInputs(500, 10000, rng)
		_ = genex.Genex(inputs)
	}
}

func BenchmarkGenexRandom_1000x10000(b *testing.B) {
	rng := rand.New(rand.NewSource(42))
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		inputs := generateRandomInputs(1000, 10000, rng)
		_ = genex.Genex(inputs)
	}
}
