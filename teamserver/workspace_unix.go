//go:build !windows

package main

import (
	"fmt"
	"os"
	"path/filepath"
	"syscall"
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
	if st, ok := info.Sys().(*syscall.Stat_t); ok {
		if st.Uid != uint32(os.Getuid()) {
			return fmt.Errorf("workspace directory %s owned by uid %d (expected %d)", dir, st.Uid, os.Getuid())
		}
	}
	if info.Mode().Perm()&0o077 != 0 {
		if chErr := os.Chmod(dir, 0o700); chErr != nil {
			return fmt.Errorf("failed securing workspace directory %s permissions: %w", dir, chErr)
		}
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
	fd, err := syscall.Open(path, syscall.O_WRONLY|syscall.O_CREAT|syscall.O_TRUNC|syscall.O_NOFOLLOW, uint32(perm))
	if err != nil {
		return err
	}
	f := os.NewFile(uintptr(fd), path)
	if f == nil {
		_ = syscall.Close(fd)
		return fmt.Errorf("failed wrapping file descriptor for %s", path)
	}
	_, writeErr := f.Write(data)
	closeErr := f.Close()
	if writeErr != nil {
		return writeErr
	}
	return closeErr
}
