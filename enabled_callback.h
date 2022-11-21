#pragma once

#include <map>
#include <SDK/foobar2000-lite.h>
#include <SDK/foobar2000-all.h>

namespace pauldsp {
	
	struct unregister_callback
	{
		std::function<void()> destroyer;
		unregister_callback(std::function<void()> destroyer) : destroyer(destroyer) {}
		void unregister() { destroyer(); }
	};

	struct callback_deletor
	{
		void operator()(unregister_callback* callback)
		{
			callback->unregister();
			delete callback;
		}
	};

	// This class should only be called from the main thread, and the callbacks should
	// ensure they run in the main thread as well. (there's no thread safety here).
	//
	// Remember to only update UI state - no core dsp changes.
	class enabled_callback : public dsp_config_callback
	{
		std::map<size_t, std::function<void()>> myIDs;
		size_t curID;

	public:

		enabled_callback() : myIDs(), curID(0) {}

	private:

		std::function<void()> destroyer(size_t id)
		{
			// since this is a static factory service, a reference
			// to the map should be fine since it should only be
			// undefined when foobar is exiting - in which case, the UI
			// shouldn't be making calls anyway?
			return [&, id]() -> void { 
				myIDs.erase(id); 
			};
		}

		size_t getID() 
		{
			return ++curID;
		}

	public:

		std::unique_ptr<unregister_callback, callback_deletor> registerCallback(std::function<void()> f)
		{
			std::unique_ptr<unregister_callback, callback_deletor> toReturn{};
			if (!core_api::is_main_thread())
				return toReturn;

			// In case we have a lot of ui_instances of this in the future for some reason,
			// we'll do a little check to make sure we have unique ids for 100 or less elements.
			size_t id = getID();
			for (size_t trys = 0; myIDs.find(id) != myIDs.end() && trys < 100; ++trys)
				id = getID();

			myIDs.emplace(id, f);
			unregister_callback* newCallback = new unregister_callback(destroyer(id));
			toReturn = std::unique_ptr<unregister_callback, callback_deletor>(newCallback);

			return toReturn;
		}

		void on_core_settings_change(const dsp_chain_config& p_newdata) override
		{
			fb2k::inMainThread2([&]() -> void {
				for (auto it = myIDs.cbegin(); it != myIDs.cend(); ++it)
				{
					it->second();
				}
			});
		}
	};
}
