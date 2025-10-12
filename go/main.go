package main

import (
	"fmt"
)

func main() {
	// Test case 1: Simple common substring
	fmt.Println("Test 1: Simple common substring")
	callProcess([]string{
		"hello world",
		"hello there",
		"hello go",
	})
	fmt.Println()

	// Test case 2: Multiple common substrings
	fmt.Println("Test 2: Multiple common substrings")
	callProcess([]string{
		"the quick brown fox",
		"the slow brown dog",
		"the fast brown cat",
	})
	fmt.Println()

	// Test case 3: Paths
	fmt.Println("Test 3: File paths")
	callProcess([]string{
		"/usr/local/bin/app",
		"/usr/local/lib/app",
		"/usr/local/share/app",
	})
	fmt.Println()

	// Test case 4: URLs
	fmt.Println("Test 4: URLs")
	callProcess([]string{
		"https://example.com/api/v1/users",
		"https://example.com/api/v1/posts",
		"https://example.com/api/v1/comments",
	})
	fmt.Println()

	// Test case 5: No common substring
	fmt.Println("Test 5: No common substring")
	callProcess([]string{
		"abc",
		"def",
		"ghi",
	})
	fmt.Println()
}
