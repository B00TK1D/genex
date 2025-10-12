package main

import (
	"fmt"
	genex "github.com/B00TK1D/genex"
)

func printProtocol(p genex.Protocol) {
	for i, field := range p.Field {
		empty := false
		if i == 0 {
			empty = true
			for _, b := range field.Variables {
				if len(b) > 0 {
					empty = false
					break
				}
			}
		}
		if !empty {
			fmt.Printf("(")
			for j, variable := range field.Variables {
				if j > 0 {
					fmt.Printf("|")
				}
				fmt.Printf("%s", variable)
			}
			fmt.Printf(")")
		}
		fmt.Printf("%s", field.Constant)
	}
}

func main() {
	// Test case 1: Simple common substring
	fmt.Println("Test 1: Simple common substring")
	printProtocol(genex.Genex([][]byte{
		[]byte("hello world"),
		[]byte("hello there"),
		[]byte("hello go"),
	}))
	fmt.Println()

	// Test case 2: Multiple common substrings
	fmt.Println("Test 2: Multiple common substrings")
	printProtocol(genex.Genex([][]byte{
		[]byte("the quick brown fox"),
		[]byte("the slow brown dog"),
		[]byte("the fast brown cat"),
	}))
	fmt.Println()

	// Test case 3: Paths
	fmt.Println("Test 3: File paths")
	printProtocol(genex.Genex([][]byte{
		[]byte("/usr/local/bin/app"),
		[]byte("/usr/local/lib/app"),
		[]byte("/usr/local/share/app"),
	}))
	fmt.Println()

	// Test case 4: URLs
	fmt.Println("Test 4: URLs")
	printProtocol(genex.Genex([][]byte{
		[]byte("https://example.com/api/v1/users"),
		[]byte("https://example.com/api/v1/posts"),
		[]byte("https://example.com/api/v1/comments"),
	}))
	fmt.Println()

	// Test case 5: No common substring
	fmt.Println("Test 5: No common substring")
	printProtocol(genex.Genex([][]byte{
		[]byte("abc"),
		[]byte("def"),
		[]byte("ghi"),
	}))
	fmt.Println()
}
