#include <server/QRPeer.h>
#include <tasks/tasks.h>
namespace MM {
    bool PerformGetGameInfo(MMPushRequest request, TaskThreadData  *thread_data) {
		OS::GameData game_info;
        if(!thread_data->shared_game_cache->LookupGameByName(request.gamename.c_str(), game_info)) {
            game_info = OS::GetGameByName(request.gamename.c_str(), thread_data->mp_redis_connection);
            thread_data->shared_game_cache->AddGame(thread_data->id, game_info);
        }
		
		if(request.peer) {
            request.peer->OnGetGameInfo(game_info, request.state);
			request.peer->DecRef();
		}
        return true;
    }
}