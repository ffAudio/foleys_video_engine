/*
 ==============================================================================

    Copyright (c) 2019 - 2021, Foleys Finest Audio - Daniel Walz
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

namespace foleys
{

ControllableBase::ControllableBase (TimeCodeAware& reference)
  : timeReference (reference)
{
}

void ControllableBase::notifyParameterAutomationChange (const ParameterAutomation* p)
{
    listeners.call ([p](auto& l) { l.parameterAutomationChanged (p); });
}

void ControllableBase::addListener (Listener* listener)
{
    listeners.add (listener);
}

void ControllableBase::removeListener (Listener* listener)
{
    listeners.remove (listener);
}

TimeCodeAware& ControllableBase::getTimeReference()
{
    return timeReference;
}

const TimeCodeAware& ControllableBase::getTimeReference() const
{
    return timeReference;
}


} // namespace foleys
