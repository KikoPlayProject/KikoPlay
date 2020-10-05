#ifndef PEERID_H
#define PEERID_H
#include <QByteArray>
#include <QString>
#include <QHash>
// https://github.com/webtorrent/bittorrent-peerid
namespace PeerId
{
using VersionFunc = QString (*)(const QByteArray &);
QString VER_AZ_THREE_DIGITS(const QByteArray &v) // "1.2.3"
{
    return QString("%1.%2.%3").arg(v[0]).arg(v[1]).arg(v[2]);
}
QString VER_AZ_DELUGE(const QByteArray &v)
{
  if (v[2]>='A' && v[2]<='E')
      return QString("%1.%2.1%3").arg(v[0]).arg(v[1]).arg(v[2]-'A');
  return QString("%1.%2.%3").arg(v[0]).arg(v[1]).arg(v[2]);
}
QString VER_AZ_THREE_DIGITS_PLUS_MNEMONIC(const QByteArray &v) // "1.2.3 [4]"
{
    auto mnemonic = v[3];
    if (mnemonic == 'B') {
        return QString("%1.%2.%3 Beta").arg(v[0]).arg(v[1]).arg(v[2]);
    } else if (mnemonic == 'A') {
        return QString("%1.%2.%3 Alpha").arg(v[0]).arg(v[1]).arg(v[2]);
    }
    return QString("%1.%2.%3").arg(v[0]).arg(v[1]).arg(v[2]);
}
QString VER_AZ_FOUR_DIGITS(const QByteArray &v) // "1.2.3.4"
{
    return QString("%1.%2.%3.%4").arg(v[0]).arg(v[1]).arg(v[2]).arg(v[3]);
}
QString VER_AZ_TWO_MAJ_TWO_MIN(const QByteArray &v) // "12.34"
{
   return QString("%1%2.%3%4").arg(v[0]).arg(v[1]).arg(v[2]).arg(v[3]);
}
QString VER_AZ_SKIP_FIRST_ONE_MAJ_TWO_MIN(const QByteArray &v) // "2.34"
{
    return QString("%1.%2%3").arg(v[1]).arg(v[2]).arg(v[3]);
}
QString VER_AZ_KTORRENT_STYLE(const QByteArray &)
{
    return "1.2.3=[RD].4";
}
QString VER_AZ_TRANSMISSION_STYLE(const QByteArray &v)   // "transmission"
{
    if (v[0] == '0' && v[1] == '0' && v[2] == '0') {
        return QString("0.%1").arg(v[3]);
    } else if (v[0] == '0' && v[1] == '0') {
        return QString("0.%1%2").arg(v[2]).arg(v[3]);
    }
    return QString("%1.%2%3%4").arg(v[0]).arg(v[1]).arg(v[2]).arg(v[3]=='Z'||v[3]=='X'?'+':' ');
}
QString VER_AZ_WEBTORRENT_STYLE(const QByteArray &v)  // "webtorrent"
{
    QString version;
    if (v[0] == '0') {
        version += v[1];
    } else {
        version += v[0];
        version += v[1];
    }
    if (v[2] == '0') {
        version += v[3];
    } else {
        version += v[2];
        version += v[3];
    }
    return version;
}
QString VER_AZ_THREE_ALPHANUMERIC_DIGITS(const QByteArray &)
{
    return "2.33.4";
}
QString VER_NONE(const QByteArray &)
{
    return "NO_VERSION";
}

static QHash<QString, QPair<QString, VersionFunc>> azStyleClients
{
    {"A~", {"Ares", VER_AZ_THREE_DIGITS}},
    {"AG", {"Ares", VER_AZ_THREE_DIGITS}},
    {"AN", {"Ares", VER_AZ_FOUR_DIGITS}},
    {"AR", {"Ares",  VER_AZ_FOUR_DIGITS}}, // Ares is more likely than ArcticTorrent
    {"AV", {"Avicora",  VER_AZ_FOUR_DIGITS}},
    {"AX", {"BitPump", VER_AZ_TWO_MAJ_TWO_MIN}},
    {"AT", {"Artemis",  VER_AZ_FOUR_DIGITS}},
    {"AZ", {"Vuze", VER_AZ_FOUR_DIGITS}},
    {"BB", {"BitBuddy",  VER_AZ_FOUR_DIGITS}},
    {"BC", {"BitComet", VER_AZ_SKIP_FIRST_ONE_MAJ_TWO_MIN}},
    {"BE", {"BitTorrent SDK",  VER_AZ_FOUR_DIGITS}},
    {"BF", {"BitFlu", VER_NONE}},
    {"BG", {"BTG", VER_AZ_FOUR_DIGITS}},
    {"bk", {"BitKitten (libtorrent)",  VER_AZ_FOUR_DIGITS}},
    {"BR", {"BitRocket",  VER_AZ_FOUR_DIGITS}},
    {"BS", {"BTSlave",  VER_AZ_FOUR_DIGITS}},
    {"BT", {"BitTorrent", VER_AZ_THREE_DIGITS_PLUS_MNEMONIC}},
    {"BW", {"BitWombat",  VER_AZ_FOUR_DIGITS}},
    {"BX", {"BittorrentX",  VER_AZ_FOUR_DIGITS}},
    {"CB", {"Shareaza Plus",  VER_AZ_FOUR_DIGITS}},
    {"CD", {"Enhanced CTorrent", VER_AZ_TWO_MAJ_TWO_MIN}},
    {"CT", {"CTorrent",  VER_AZ_FOUR_DIGITS}},
    {"DP", {"Propogate Data Client",  VER_AZ_FOUR_DIGITS}},
    {"DE", {"Deluge", VER_AZ_DELUGE}},
    {"EB", {"EBit",  VER_AZ_FOUR_DIGITS}},
    {"ES", {"Electric Sheep", VER_AZ_THREE_DIGITS}},
    {"FC", {"FileCroc",  VER_AZ_FOUR_DIGITS}},
    {"FG", {"FlashGet", VER_AZ_SKIP_FIRST_ONE_MAJ_TWO_MIN}},
    {"FX", {"Freebox BitTorrent",  VER_AZ_FOUR_DIGITS}},
    {"FT", {"FoxTorrent/RedSwoosh",  VER_AZ_FOUR_DIGITS}},
    {"GR", {"GetRight",  VER_AZ_FOUR_DIGITS}},
    {"GS", {"GSTorrent",  VER_AZ_FOUR_DIGITS}},// TODO: Format is v"abcd"
    {"HL", {"Halite", VER_AZ_THREE_DIGITS}},
    {"HN", {"Hydranode",  VER_AZ_FOUR_DIGITS}},
    {"KG", {"KGet",  VER_AZ_FOUR_DIGITS}},
    {"KT", {"KTorrent", VER_AZ_KTORRENT_STYLE}},
    {"LC", {"LeechCraft",  VER_AZ_FOUR_DIGITS}},
    {"LH", {"LH-ABC",  VER_AZ_FOUR_DIGITS}},
    {"LK", {"linkage", VER_AZ_THREE_DIGITS}},
    {"LP", {"Lphant", VER_AZ_TWO_MAJ_TWO_MIN}},
    {"LT", {"libtorrent (Rasterbar)", VER_AZ_THREE_ALPHANUMERIC_DIGITS}},
    {"lt", {"libTorrent (Rakshasa)", VER_AZ_THREE_ALPHANUMERIC_DIGITS}},
    {"LW", {"LimeWire", VER_NONE}},// The "0001" bytes found after the LW commonly refers to the version of the BT protocol implemented. Documented here: http://www.limewire.org/wiki/index.php?title=BitTorrentRevision
    {"MO", {"MonoTorrent",  VER_AZ_FOUR_DIGITS}},
    {"MP", {"MooPolice", VER_AZ_THREE_DIGITS}},
    {"MR", {"Miro",  VER_AZ_FOUR_DIGITS}},
    {"MT", {"MoonlightTorrent",  VER_AZ_FOUR_DIGITS}},
    {"NE", {"BT Next Evolution", VER_AZ_THREE_DIGITS}},
    {"NX", {"Net Transport",  VER_AZ_FOUR_DIGITS}},
    {"OS", {"OneSwarm", VER_AZ_FOUR_DIGITS}},
    {"OT", {"OmegaTorrent",  VER_AZ_FOUR_DIGITS}},
    {"PC", {"CacheLogic",  VER_AZ_FOUR_DIGITS}},
    {"PT", {"Popcorn Time",  VER_AZ_FOUR_DIGITS}},
    {"PD", {"Pando",  VER_AZ_FOUR_DIGITS}},
    {"PE", {"PeerProject",  VER_AZ_FOUR_DIGITS}},
    {"pX", {"pHoeniX",  VER_AZ_FOUR_DIGITS}},
    {"qB", {"qBittorrent", VER_AZ_DELUGE}},
    {"QD", {"qqdownload",  VER_AZ_FOUR_DIGITS}},
    {"RT", {"Retriever",  VER_AZ_FOUR_DIGITS}},
    {"RZ", {"RezTorrent",  VER_AZ_FOUR_DIGITS}},
    {"S~", {"Shareaza alpha/beta",  VER_AZ_FOUR_DIGITS}},
    {"SB", {"SwiftBit",  VER_AZ_FOUR_DIGITS}},
    {"SD", {"Xunlei",  VER_AZ_FOUR_DIGITS}},// Apparently, the English name of the client is "Thunderbolt".
    {"SG", {"GS Torrent", VER_AZ_FOUR_DIGITS}},
    {"SN", {"ShareNET",  VER_AZ_FOUR_DIGITS}},
    {"SP", {"BitSpirit", VER_AZ_THREE_DIGITS}},// >= 3.6
    {"SS", {"SwarmScope",  VER_AZ_FOUR_DIGITS}},
    {"ST", {"SymTorrent",  VER_AZ_FOUR_DIGITS}},
    {"st", {"SharkTorrent",  VER_AZ_FOUR_DIGITS}},
    {"SZ", {"Shareaza",  VER_AZ_FOUR_DIGITS}},
    {"TG", {"Torrent GO",  VER_AZ_FOUR_DIGITS}},
    {"TN", {"Torrent.NET",  VER_AZ_FOUR_DIGITS}},
    {"TR", {"Transmission", VER_AZ_TRANSMISSION_STYLE}},
    {"TS", {"TorrentStorm",  VER_AZ_FOUR_DIGITS}},
    {"TT", {"TuoTu", VER_AZ_THREE_DIGITS}},
    {"UL", {"uLeecher!",  VER_AZ_FOUR_DIGITS}},
    {"UE", {QChar(0x00B5)+QString("Torrent Embedded"), VER_AZ_THREE_DIGITS_PLUS_MNEMONIC}},
    {"UT", {QChar(0x00B5)+QString("Torrent"), VER_AZ_THREE_DIGITS_PLUS_MNEMONIC}},
    {"UM", {QChar(0x00B5)+QString("Torrent Mac"), VER_AZ_THREE_DIGITS_PLUS_MNEMONIC}},
    {"UW", {QChar(0x00B5)+QString("Torrent Web"), VER_AZ_THREE_DIGITS_PLUS_MNEMONIC}},
    {"WD", {"WebTorrent Desktop", VER_AZ_WEBTORRENT_STYLE}},// Go Webtorrent!! :}},
    {"WT", {"Bitlet",  VER_AZ_FOUR_DIGITS}},
    {"WW", {"WebTorrent", VER_AZ_WEBTORRENT_STYLE}},// Go Webtorrent!! :}},
    {"WY", {"FireTorrent",  VER_AZ_FOUR_DIGITS}},// formerly Wyzo.
    {"VG", {"Vagaa", VER_AZ_FOUR_DIGITS}},
    {"XL", {"Xunlei",  VER_AZ_FOUR_DIGITS}},// Apparently, the English name of the client is "Thunderbolt".
    {"XT", {"XanTorrent",  VER_AZ_FOUR_DIGITS}},
    {"XF", {"Xfplay", VER_AZ_TRANSMISSION_STYLE}},
    {"XX", {"XTorrent",  VER_AZ_FOUR_DIGITS}},
    {"XC", {"XTorrent",  VER_AZ_FOUR_DIGITS}},
    {"ZT", {"ZipTorrent",  VER_AZ_FOUR_DIGITS}},
    {"7T", {"aTorrent",  VER_AZ_FOUR_DIGITS}},
    {"ZO", {"Zona", VER_AZ_FOUR_DIGITS}},
    {"#@", {"Invalid PeerID",  VER_AZ_FOUR_DIGITS}}
};
static QHash<QChar, QString> shadowStyleClients{
    {'A', "ABC"},
    {'O', "Osprey Permaseed"},
    {'Q', "BTQueue"},
    {'R', "Tribler"},
    {'S', "Shad0w"},
    {'T', "BitTornado"},
    {'U', "UPnP NAT"}
};
static QHash<QChar, QString> mainlineStyleClients  {
    {'M', "Mainline"},
    {'Q', "Queen Bee"}
};
struct SimpleClientInfo
{
    QString client, version, id;
    int pos;
};
QList<SimpleClientInfo> customStyleClients {
      {QChar(0x00B5)+QString("Torrent"), "1.7.0 RC", "-UT170-", 0},// http://forum.utorrent.com/viewtopic.php?pid=260927#p260927
      {"Azureus", "1", "Azureus", 0},
      {"Azureus", "2.0.3.2", "Azureus", 5},
      {"Aria", "2", "-aria2-", 0},
      {"BitTorrent Plus!", "II", "PRC.P---", 0},
      {"BitTorrent Plus!", "", "P87.P---", 0},
      {"BitTorrent Plus!", "", "S587Plus", 0},
      {"BitTyrant (Azureus Mod)", "", "AZ2500BT", 0},
      {"Blizzard Downloader", "", "BLZ", 0},
      {"BTGetit", "", "BG", 10},
      {"BTugaXP", "", "btuga", 0},
      {"BTugaXP", "", "BTuga", 5},
      {"BTugaXP", "", "oernu", 0},
      {"Deadman Walking", "", "BTDWV-", 0},
      {"Deadman", "", "Deadman Walking-", 0},
      {"External Webseed", "", "Ext", 0},
      {"G3 Torrent", "", "-G3", 0},
      {"GreedBT", "2.7.1", "271-", 0},
      {"Hurricane Electric", "", "arclight", 0},
      {"HTTP Seed", "", "-WS", 0},
      {"JVtorrent", "", "10-------", 0},
      {"Limewire", "", "LIME", 0},
      {"Martini Man", "", "martini", 0},
      {"Pando", "", "Pando", 0},
      {"PeerApp", "", "PEERAPP", 0},
      {"SimpleBT", "", "btfans", 4},
      {"Swarmy", "", "a00---0", 0},
      {"Swarmy", "", "a02---0", 0},
      {"Teeweety", "", "T00---0", 0},
      {"TorrentTopia", "", "346-", 0},
      {"XanTorrent", "", "DansClient", 0},
      {"MediaGet", "", "-MG1", 0},
      {"MediaGet", "2.1", "-MG21", 0},
      {"Amazon AWS S3", "", "S3-", 0},
      {"BitTorrent DNA", "", "DNA", 0},
      {"Opera", "", "OP", 0},// Pre build 10000 versions
      {"Opera", "", "O", 0},// Post build 10000 versions
      {"Burst!", "", "Mbrst", 0},
      {"TurboBT", "", "turbobt", 0},
      {"BT Protocol Daemon", "", "btpd", 0},
      {"Plus!", "", "Plus", 0},
      {"XBT", "", "XBT", 0},
      {"BitsOnWheels", "", "-BOW", 0},
      {"eXeem", "", "eX", 0},
      {"MLdonkey", "", "-ML", 0},
      {"Bitlet", "", "BitLet", 0},
      {"AllPeers", "", "AP", 0},
      {"BTuga Revolution", "", "BTM", 0},
      {"Rufus", "", "RS", 2},
      {"BitMagnet", "", "BM", 2},// BitMagnet - predecessor to Rufus
      {"QVOD", "", "QVOD", 0},
      // Top-BT is based on BitTornado, but doesn"t quite stick to Shadow"s naming conventions,
      // so we"ll use substring matching instead.
      {"Top-BT", "", "TB", 0},
      {"Tixati", "", "TIX", 0},
      // seems to have a sub-version encoded in following 3 bytes, not worked out how: "folx/1.0.456.591" : 2D 464C 3130 FF862D 486263574A43585F66314D5A
      {"folx", "", "-FL", 0},
      {QChar(0x00B5)+QString("Torrent Mac"), "", "-UM", 0},
      {QChar(0x00B5)+QString("Torrent"), "", "-UT", 0} // UT 3.4+
};
bool isPossibleSpoofClient(const QByteArray &peerId)
{
    return peerId.endsWith("UDP0") || peerId.endsWith("HTTPBT");
}
QString decodeBitSpiritClient(const QByteArray &peerId)
{
    if (peerId.mid(2, 2) != "BS") return QString();
    auto version = peerId[1];
    if (version == '0') version = '1';
    return QString("BitSpirit %1").arg(version);
}
QString decodeBitCometClient(const QByteArray &peerId)
{
    QString modName;
    if (peerId.startsWith("exbc")) modName = "";
    else if (peerId.startsWith("FUTB")) modName = "(Solidox Mod)";
    else if (peerId.startsWith("xUTB")) modName = "(Mod 2)";
    else return QString();

    bool isBitlord = (peerId.mid(6, 4) == "LORD");
    // Older versions of BitLord are of the form x.yy, whereas new versions (1 and onwards),
    // are of the form x.y. BitComet is of the form x.yy
    auto clientName = (isBitlord) ? "BitLord" : "BitComet";
    return QString("%1%2 %3.%4").arg(clientName, modName.isEmpty()?"":" "+modName).arg(peerId[4], peerId[5]);
}
bool isAzStyle (const QByteArray &peerId)
{
    if (peerId[0] != '-') return false;
    if (peerId[7] == '-') return true;

    /**
     * Hack for FlashGet - it doesn't use the trailing dash.
     * Also, LH-ABC has strayed into "forgetting about the delimiter" territory.
     *
     * In fact, the code to generate a peer ID for LH-ABC is based on BitTornado's,
     * yet tries to give an Az style peer ID... oh dear.
     *
     * BT Next Evolution seems to be in the same boat as well.
     *
     * KTorrent 3 appears to use a dash rather than a final character.
     */
    auto m = peerId.mid(1, 2);
    if (m == "FG" || m == "LH" || m == "NE" || m == "KT" || m == "SP") return true;
    return false;
}
bool isShadowStyle(const QByteArray &peerId)
{
    if (peerId[5] != '-') return false;
    if (!QChar::isLetter(peerId[0])) return false;
    if (!(QChar::isDigit(peerId[1]) || peerId[1] == '-')) return false;
    // Find where the version number string ends.
    int lastVersionNumberIndex = 4;
    for (; lastVersionNumberIndex > 0; lastVersionNumberIndex--) {
      if (peerId[lastVersionNumberIndex] != '-') break;
    }
    // For each digit in the version string, check if it is a valid version identifier.
    for (int i = 1; i <= lastVersionNumberIndex; i++) {
      if (peerId[i] == '-') return false;
      if (!(QChar::isLetterOrNumber(peerId[i]) || peerId[i]=='.')) return false;
    }
    return true;
}
bool isMainlineStyle (const QByteArray &peerId) {
    /**
     * One of the following styles will be used:
     *   Mx-y-z--
     *   Mx-yy-z-
     */
    return peerId[2] == '-' && peerId[7] == '-' && (peerId[4] == '-' || peerId[5] == '-');
}
QString getSimpleClient (const QByteArray &peerId)
{
    for (auto &client : customStyleClients) {
      if(peerId.mid(client.pos).startsWith(client.id.toLatin1()))
          return QString("%1 %2").arg(client.client, client.version);
    }
    return QString();
}
QString identifyAwkwardClient(const QByteArray &peerId) {
    int firstNonZeroIndex = 20;
    int i = 0;
    for (; i < 20; ++i)
    {
        if (peerId[i] > 0)
        {
            firstNonZeroIndex = i;
            break;
        }
    }
    bool isShareaza = true;
    // Shareaza check
    if (firstNonZeroIndex == 0)
    {
        for (i = 0; i < 16; ++i)
        {
            if (peerId[i] == 0)
            {
                isShareaza = false;
                break;
            }
        }

        if (isShareaza)
        {
            for (i = 16; i < 20; ++i)
            {
                if (peerId[i] != (peerId[i % 16] ^ peerId[15 - (i % 16)]))
                {
                    isShareaza = false;
                    break;
                }
            }

            if (isShareaza) return "Shareaza";
        }
    }
    if (firstNonZeroIndex == 9 && peerId[9] == 3 && peerId[10] == 3 && peerId[11] == 3)
        return "I2PSnark";
    if (firstNonZeroIndex == 12 && peerId[12] == 97 && peerId[13] == 97)
        return "Experimental 3.2.1b2";
    if (firstNonZeroIndex == 12 && peerId[12] == 0 && peerId[13] == 0)
        return "Experimental 3.1";
    if (firstNonZeroIndex == 12)
        return "Mainline";
    return "";
}
QString convertPeerId(const QByteArray &peerId)
{
    if(peerId.size()!=20) return peerId;
    if(isPossibleSpoofClient(peerId))
    {
        QString client(decodeBitSpiritClient(peerId));
        if(client.isEmpty()) client = decodeBitCometClient(peerId);
        if(client.isEmpty()) client = "BitSpirit?";
        return client;
    }
    if (isAzStyle(peerId))
    {
        if(azStyleClients.contains(peerId.mid(1,2)))
        {
            auto &cv = azStyleClients[peerId.mid(1,2)];
            QString version = cv.second(peerId.mid(3, 4));
            if(cv.first.startsWith("ZipTorrent") && peerId.mid(8, 5) == "bLAde")
            {
                return QString("Unknown [Fake: ZipTorrent] %1").arg(version);
            }
            if(cv.first == QChar(0x00B5)+QString("Torrent") && version == "6.0 Beta")
            {
                return "Mainline 6.0 Beta";
            }
            if(cv.first.startsWith("libTorrent (Rakshasa)"))
            {
                return QString("%1 / rTorrent* %2").arg(cv.first, version);
            }
            return QString("%1 %2").arg(cv.first, version);
        }
      }
    if(isShadowStyle(peerId))
    {
        if(shadowStyleClients.contains(peerId[0]))
            return shadowStyleClients[peerId[0]];
    }
    if(isMainlineStyle(peerId))
    {
        if(mainlineStyleClients.contains(peerId[0]))
            return mainlineStyleClients[peerId[0]];
    }
    QString client(decodeBitSpiritClient(peerId));
    if(!client.isEmpty()) return client;
    client = decodeBitCometClient(peerId);
    if (!client.isEmpty()) return client;
    client = getSimpleClient(peerId);
    if (!client.isEmpty()) return client;
    client = identifyAwkwardClient(peerId);
    if (!client.isEmpty()) return client;
    return peerId;
}
}
#endif // PEERID_H
