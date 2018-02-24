#pragma once
#include <string>
#include <stdexcept>

class APRSPacket
{
    uint8_t sender_ssid;
    std::string sender_callsign;
    uint8_t target_ssid;
    uint8_t send_path;
    std::string custom_data;

  public:
    APRSPacket(
        uint8_t sender_ssid,
        const std::string &sender_callsign,
        uint8_t target_ssid,
        uint8_t send_path,
        const std::string &custom_data = "")
        : sender_ssid(sender_ssid),
          sender_callsign(sender_callsign),
          target_ssid(target_ssid),
          send_path(send_path),
          custom_data(custom_data)
    {
    }

    std::string ToString() const
    {
        if (sender_callsign.size() != 6)
        {
            throw std::runtime_error("sender_callsign should be exactly 6 letters long");
        }

        std::string packet;

        // this will speed shit up a notch
        packet.reserve(128);

        // some starting markers
        // shifted right because at the end we shift everything to the left
        packet.insert(packet.end(), 1, 0x7E >> 1);

        // APRS destination
        appendString(packet, "APZQ00");

        // SSID of the destination
        packet.push_back(0b01110000 | target_ssid); // 0b0CRRSSID (C - command/response bit '1', RR - reserved '11', SSID - 0-15)

        // Source Address
        appendString(packet, sender_callsign);

        // SSID to specify an APRS symbol (11 - balloon)
        packet.push_back(0b00110000 | (sender_ssid & 0x0F)); // 0b0CRRSSID (C - command/response bit '1', RR - reserved '11', SSID - 0-15)

        appendString(packet, "WIDE");

        packet.push_back(send_path + '0');
        packet.push_back(' ');
        packet.push_back(0b00110001); // 0b0HRRSSID (H - 'has been repeated' bit, RR - reserved '11', SSID - 0-15)

        // left shift the address bytes
        for (auto &byte : packet)
        {
            byte <<= 1;
        }

        // the last byte's LSB set to '1' to indicate the end of the address fields
        packet.back() |= 0x01;

        // Control Field
        packet.push_back(0x03);

        // Protocol ID
        packet.push_back(0xF0);

        // Information Field
        packet.push_back('@');
        appendString(packet, timestr());

        appendString(packet, "0000.00N/00000.00W.");

        appendString(packet, custom_data);

        // Frame Check Sequence - CRC-16-CCITT (0xFFFF)
        uint16_t crc = 0xFFFF;
        for (uint16_t i = 1; i < packet.size(); i++)
        {
            crc = crc_ccitt_update(crc, packet[i]);
        }

        crc = ~crc;
        packet.push_back(crc & 0xFF);        // FCS is sent low-byte first
        packet.push_back((crc >> 8) & 0xFF); // and with the bits flipped

        // the end Flags
        packet.insert(packet.end(), 1, 0x7E);
        return packet;
    }

  private:
    // Appends a string without the trailing zero
    template <typename Cont>
    static void appendString(Cont &container, const std::string &data)
    {
        container.insert(container.end(), std::begin(data), std::end(data));
    }

    static uint16_t crc_ccitt_update(uint16_t crc, uint8_t data)
    {
        data ^= crc & 0xff;
        data ^= data << 4;

        return ((((uint16_t)data << 8) | (crc >> 8)) ^ (uint8_t)(data >> 4) ^ ((uint16_t)data << 3));
    }

    static std::string timestr()
    {
        time_t rawtime;
        struct tm *timeinfo;
        char buffer[10];

        time(&rawtime);
        timeinfo = localtime(&rawtime);

        strftime(buffer, sizeof(buffer), "%H%M%Sh", timeinfo);
        return buffer;
    }
};
