// tar_split_json writes tar-split metadata without choosing its compression.
package main

import (
	"fmt"
	"io"
	"os"

	"github.com/vbatts/tar-split/tar/asm"
	"github.com/vbatts/tar-split/tar/storage"
)

func writeMetadata(path string) (err error) {
	output, err := os.OpenFile(path, os.O_CREATE|os.O_WRONLY|os.O_TRUNC, 0600)
	if err != nil {
		return err
	}
	defer func() {
		if closeErr := output.Close(); err == nil {
			err = closeErr
		}
	}()
	stream, err := asm.NewInputTarStream(os.Stdin, storage.NewJSONPacker(output), nil)
	if err != nil {
		return err
	}
	_, err = io.Copy(io.Discard, stream)
	return err
}

func main() {
	if len(os.Args) != 2 {
		fmt.Fprintln(os.Stderr, "usage: tar_split_json METADATA_PATH < archive.tar")
		os.Exit(2)
	}
	if err := writeMetadata(os.Args[1]); err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(1)
	}
}
