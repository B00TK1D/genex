# genex

Generate tightly-fitting protocols from input sets using recursive longest substring division with minimized constant distance.

## Example

```go
package main

import (
	"fmt"
	genex "github.com/B00TK1D/genex"
)

func main() {
	proto := genex.Genex([][]byte{
		[]byte("the quick brown fox"),
		[]byte("the slow brown dog"),
		[]byte("the fast brown cat"),
	})

    for _, field := range proto.Field {
		fmt.Printf("(")
		for j, variable := range field.Variables {
			if j > 0 {
				fmt.Printf("|")
			}
			fmt.Printf("%s", variable)
		}
		fmt.Printf(")%s", field.Constant)
	}
}
```

Output:
```
(||)the (quick|slow|fast) brown (fox|dog|cat)
```
