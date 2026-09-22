#include "StartModule.h"

#include "config.h"

StartSignal StartModule::read() {
  if (!receiver_.decode(&results_)) {
    return StartSignal::None;
  }
  const uint32_t code = results_.value;
  receiver_.resume();

  switch (code) {
    case config::kIrCodeReady:
      return StartSignal::Ready;
    case config::kIrCodeStart:
      return StartSignal::Start;
    case config::kIrCodeStop:
      return StartSignal::Stop;
    default:
      return StartSignal::None;
  }
}
