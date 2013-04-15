// playermanager.cpp
// Created 6/1/2010 by RJ Ryan (rryan@mit.edu)

#include "playermanager.h"

#include "trackinfoobject.h"
#include "player.h"
#include "analyserqueue.h"
#include "controlobject.h"
#include "library/library.h"
#include "library/trackcollection.h"
#include "engine/enginemaster.h"

PlayerManager::PlayerManager(ConfigObject<ConfigValue> *pConfig,
                             EngineMaster* pEngine,
                             Library* pLibrary)
        : m_pConfig(pConfig),
          m_pEngine(pEngine),
          m_pLibrary(pLibrary) {
    m_pAnalyserQueue = AnalyserQueue::createDefaultAnalyserQueue(pConfig);

    connect(m_pLibrary, SIGNAL(loadTrackToPlayer(TrackPointer, int)),
            this, SLOT(slotLoadTrackToPlayer(TrackPointer, int)));
    connect(m_pLibrary, SIGNAL(loadTrack(TrackPointer)),
             this, SLOT(slotLoadTrackIntoNextAvailablePlayer(TrackPointer)));
}

PlayerManager::~PlayerManager() {
    delete m_pAnalyserQueue;

    QMutableListIterator<Player*> it(m_players);
    while (it.hasNext()) {
        Player* pPlayer = it.next();
        it.remove();
        delete pPlayer;
    }
}

int PlayerManager::numPlayers() {
    return m_players.size();
}

Player* PlayerManager::addPlayer() {
    int number = numPlayers() + 1;
    Player* pPlayer = new Player(m_pConfig, m_pEngine,
                                 number,
                                 QString("[Channel%1]").arg(number));

    // Connect the player to the library so that when a track is unloaded, it's
    // data (eg. waveform summary) is saved back to the database.
    connect(pPlayer, SIGNAL(unloadingTrack(TrackPointer)),
            &(m_pLibrary->getTrackCollection()->getTrackDAO()),
            SLOT(saveTrack(TrackPointer)));

    // Connect the player to the analyser queue so that loaded tracks are
    // analysed.
    connect(pPlayer, SIGNAL(newTrackLoaded(TrackPointer)),
            m_pAnalyserQueue, SLOT(queueAnalyseTrack(TrackPointer)));

    m_players.append(pPlayer);

    return pPlayer;
}

Player* PlayerManager::getPlayer(QString group) {
    QList<Player*>::iterator it = m_players.begin();
    while (it != m_players.end()) {
        Player* pPlayer = *it;
        if (pPlayer->getGroup() == group) {
            return pPlayer;
        }
        it++;
    }
    return NULL;
}


Player* PlayerManager::getPlayer(int player) {
    if (player < 1 || player > numPlayers()) {
        qWarning() << "Warning PlayerManager::getPlayer() called with invalid index: "
                   << player;
        return NULL;
    }
    return m_players[player - 1];
}

void PlayerManager::slotLoadTrackToPlayer(TrackPointer pTrack, int player) {
    Player* pPlayer = getPlayer(player);

    if (pPlayer == NULL) {
        qWarning() << "Invalid player argument " << player << " to slotLoadTrackToPlayer.";
        return;
    }

    pPlayer->slotLoadTrack(pTrack);
}

void PlayerManager::slotLoadTrackIntoNextAvailablePlayer(TrackPointer pTrack)
{
    QList<Player*>::iterator it = m_players.begin();
    while (it != m_players.end()) {
        Player* pPlayer = *it;
        ControlObject* playControl =
                ControlObject::getControl(ConfigKey(pPlayer->getGroup(), "play"));
        if (playControl && playControl->get() != 1.) {
            pPlayer->slotLoadTrack(pTrack, false);
            break;
        }
        it++;
    }
}

TrackPointer PlayerManager::lookupTrack(QString location) {
    // Try to get TrackPointer from library, identified by location.
    TrackDAO& trackDao = m_pLibrary->getTrackCollection()->getTrackDAO();
    TrackPointer pTrack = trackDao.getTrack(trackDao.getTrackId(location));
    // If not, create a new TrackPointer
    if (pTrack == NULL)
    {
        pTrack = TrackPointer(new TrackInfoObject(location));
    }
    return pTrack;
}

void PlayerManager::slotLoadToPlayer(QString location, int player) {
    Player* pPlayer = getPlayer(player);

    if (pPlayer == NULL) {
        qWarning() << "Invalid player argument " << player << " to slotLoadToPlayer.";
        return;
    }

    TrackPointer pTrack = lookupTrack(location);

    //Load the track into the Player.
    pPlayer->slotLoadTrack(pTrack);
}


void PlayerManager::slotLoadToPlayer(QString location, QString group) {
    Player* pPlayer = getPlayer(group);

    if (pPlayer == NULL) {
        qWarning() << "Invalid group argument " << group << " to slotLoadToPlayer.";
        return;
    }

    TrackPointer pTrack = lookupTrack(location);

    //Load the track into the Player.
    pPlayer->slotLoadTrack(pTrack);
}