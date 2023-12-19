package main

import "fmt"

const MaxUint = ^uint(0)
const MinUint = 0
const MaxInt = int(MaxUint >> 1)
const MinInt = -MaxInt - 1

type input_struct struct {
	count   int
	strings []string
}

func print_options(input input_struct, lengths []int) {
	// Print the options
	fmt.Print("(")
	for i := 0; i < input.count; i++ {
		fmt.Print(input.strings[i])
		if i < input.count-1 {
			fmt.Print("|")
		}
	}
	fmt.Print(")")
}

func print_escaped(str string, len int) {
	for i := 0; i < len; i++ {
		fmt.Print(string(str[i]))
	}
}

func longest_common_substring(input input_struct, min_len int, lengths []int, match_indices *[]int) int {
	tmp_match_indices := make([]int, input.count)
	matched_len := 0
	upper_bound := min_len
	lower_bound := 1
	subset_len := min_len
	start_index_1 := 0
	start_index_2 := 0

	for min_len > 0 {
		subset_index := input.count - 1
		subset_len = (upper_bound + lower_bound) / 2
		start_index_1 = 0
		for start_index_1 <= lengths[0]-subset_len {
			start_index_2 = 0
			subset_index = input.count - 1
			for start_index_2 <= lengths[subset_index]-subset_len && subset_index > 0 {
				if input.strings[0][start_index_1:start_index_1+subset_len] == input.strings[subset_index][start_index_2:start_index_2+subset_len] {
					tmp_match_indices[subset_index] = start_index_2
					subset_index--
					start_index_2 = 0
					continue
				}
				start_index_2++
			}
			if subset_index == 0 {
				copy(*match_indices, tmp_match_indices)
				(*match_indices)[0] = start_index_1
				matched_len = subset_len
				break
			}
			start_index_1++
		}
		if subset_index == 0 {
			lower_bound = subset_len + 1
		} else {
			upper_bound = subset_len - 1
		}
		if lower_bound > upper_bound {
			break
		}
	}
	return matched_len
}

func process(input input_struct) int {
	min_len := MaxInt
	lengths := make([]int, input.count)
	for i := 0; i < input.count; i++ {
		lengths[i] = len(input.strings[i])
		if lengths[i] < min_len {
			min_len = lengths[i]
		}
	}

	match_indices := make([]int, input.count)
	matched_len := longest_common_substring(input, min_len, lengths, &match_indices)

	if matched_len < 1 {
		print_options(input, lengths)
		return 0
	}

	nonempty := false
	recurseInput := input_struct{count: input.count, strings: make([]string, input.count)}
	for i := 0; i < input.count; i++ {
		if !nonempty && match_indices[i] > 0 {
			nonempty = true
		}
		recurseInput.strings[i] = input.strings[i][:match_indices[i]]
	}
	if nonempty {
		process(recurseInput)
	}

	print_escaped(input.strings[0][match_indices[0]:], matched_len)

	nonempty = false
	for i := 0; i < input.count; i++ {
		if !nonempty && match_indices[i]+matched_len < lengths[i] {
			nonempty = true
		}
		recurseInput.strings[i] = input.strings[i][match_indices[i]+matched_len:]
	}
	if nonempty {
		process(recurseInput)
	}

	return 0
}

func main() {
	input1 := "{'name': 'Sam Smith', 'age': 30, 'car': 'Chevy'}"
	input2 := "{'name': 'John Rogers', 'age': 25, 'car': 'Ford'}"
	input := input_struct{count: 2, strings: []string{input1, input2}}
	process(input)
}
