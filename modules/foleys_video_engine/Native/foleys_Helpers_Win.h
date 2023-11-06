/*
 ==============================================================================

 Copyright (c) 2021, Foleys Finest Audio - Daniel Walz
 All rights reserved.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 OF THE POSSIBILITY OF SUCH DAMAGE.

 ==============================================================================
 */

#pragma once


namespace foleys
{

template<typename T>
class MFScopedPointer
{
 public:
    MFScopedPointer() = default;
    ~MFScopedPointer()
    {
        reset();
    }

    /**
      Return a raw pointer to the managed pointee
     */
    T* get()
    {
        return pointer;
    }

    /**
      Returns the pointer, so a creation method can reset the pointer variable as well.
      Make sure it is nullptr before, because you cannot know if the creation succeeds or not.
     */
    T** getPointer()
    {
        jassert (pointer == nullptr);
        return &pointer;
    }

    T* operator->()
    {
        return pointer;
    }

    /**
      Set a new pointee. If there was a managed pointee it will call Release and forget it
     */
    void reset (T* p = nullptr)
    {
        if (pointer) pointer->Release();
        pointer = p;
    }

    /**
      Returns true if it has a pointee or false if it points to nullptr
     */
    operator bool() const
    {
        return pointer != nullptr;
    }

 private:
    T* pointer = nullptr;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MFScopedPointer)
};

// ==============================================================================

template<class T>
void SafeRelease (T** ppT)
{
    if (*ppT)
    {
        (*ppT)->Release();
        *ppT = NULL;
    }
}

}  // namespace foleys