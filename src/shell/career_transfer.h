#pragma once
#include "new_career.h"
namespace earthsiege {
// Expansion GO 0x16c38, before the target career is reloaded and launched.
campaign_start transfer_career(resource_store const &,campaign_start const &source,bool keep_equipment,random_state &);
}
