#pragma once

namespace WindowsHelpers {

  /** This is a very light substitute for Microsoft::WRL::ComPtr. */
  template <typename T>
  class ComPtr {
    T* _ptr;

  protected:
    void InternalAddRef() {
      if (_ptr) {
        _ptr->AddRef();
      }
    }

    void InternalRelease() {
      if (_ptr) {
        _ptr->Release();
      }
    }

  public:
    ComPtr() : _ptr(nullptr) {}
    ComPtr(ComPtr const & other) : _ptr(other._ptr) { InternalAddRef(); }
    ~ComPtr() { InternalRelease(); }

    T* operator->() const { return _ptr; }
    T** operator&() { return &_ptr; }
    T* Get() const { return _ptr; }
  };

}
