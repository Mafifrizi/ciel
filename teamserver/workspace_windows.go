//go:build windows

package main

import (
	"fmt"
	"os"
	"path/filepath"
)

func validateSecureDir(dir string) error {
	if dir == "" {
		return fmt.Errorf("workspace directory path is empty")
	}
	info, err := os.Lstat(dir)
	if os.IsNotExist(err) {
		if mkErr := os.MkdirAll(dir, 0o700); mkErr != nil {
			return mkErr
		}
		info, err = os.Lstat(dir)
	}
	if err != nil {
		return err
	}
	if info.Mode()&os.ModeSymlink != 0 {
		return fmt.Errorf("refusing symlink workspace directory: %s", dir)
	}
	if !info.IsDir() {
		return fmt.Errorf("workspace path is not a directory: %s", dir)
	}
	return nil
}

func writeFileNoFollow(path string, data []byte, perm os.FileMode) error {
	if err := validateSecureDir(filepath.Dir(path)); err != nil {
		return err
	}
	if info, err := os.Lstat(path); err == nil {
		if info.Mode()&os.ModeSymlink != 0 {
			return fmt.Errorf("refusing to write through symlink: %s", path)
		}
	} else if !os.IsNotExist(err) {
		return err
	}
	return os.WriteFile(path, data, perm)
}
