#pragma once

#include <vector>
#include <string>
#include "StyleProfiles.hpp"

namespace WorldRhythm {

// ========================================
// Regional Variants System
// ========================================
// Each style has 2-3 regional/cultural variants
// with different weight distributions and characteristics

struct RegionalVariant {
    std::string name;
    std::string region;
    StyleProfile profile;
};

// ========================================
// West African Variants
// ========================================
inline std::vector<RegionalVariant> getWestAfricanVariants() {
    std::vector<RegionalVariant> variants;

    // Mandinka (Mali/Guinea) - Djembe-centric
    {
        RegionalVariant v;
        v.name = "Mandinka";
        v.region = "Mali/Guinea";
        v.profile = {
            "West African (Mandinka)",
            {1.0f, 0.0f, 0.0f, 0.9f, 0.0f, 0.0f, 0.8f, 0.0f,
             0.0f, 0.0f, 0.85f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f},
            {0.9f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
             0.7f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
            {0.6f, 0.0f, 0.7f, 0.5f, 0.8f, 0.0f, 0.6f, 0.0f,
             0.7f, 0.0f, 0.5f, 0.0f, 0.6f, 0.4f, 0.5f, 0.0f},
            {0.5f, 0.0f, 0.0f, 0.6f, 0.0f, 0.7f, 0.0f, 0.8f,
             0.0f, 0.0f, 0.6f, 0.0f, 0.0f, 0.7f, 0.0f, 0.5f},
            0.62f, 0.3f, 0.5f, 0.1f, 0.3f, 0.4f, 0.7f, 0.2f, 0.4f,
            true, true
        };
        variants.push_back(v);
    }

    // Yoruba (Nigeria) - Dundun ensemble
    {
        RegionalVariant v;
        v.name = "Yoruba";
        v.region = "Nigeria";
        v.profile = {
            "West African (Yoruba)",
            {1.0f, 0.0f, 0.5f, 0.8f, 0.0f, 0.0f, 0.9f, 0.0f,
             0.4f, 0.0f, 0.7f, 0.0f, 0.85f, 0.0f, 0.0f, 0.0f},
            {0.95f, 0.0f, 0.0f, 0.4f, 0.0f, 0.0f, 0.0f, 0.0f,
             0.8f, 0.0f, 0.0f, 0.3f, 0.0f, 0.0f, 0.0f, 0.0f},
            {0.7f, 0.3f, 0.6f, 0.4f, 0.8f, 0.3f, 0.5f, 0.0f,
             0.6f, 0.4f, 0.7f, 0.3f, 0.5f, 0.4f, 0.6f, 0.0f},
            {0.6f, 0.0f, 0.4f, 0.5f, 0.0f, 0.6f, 0.0f, 0.7f,
             0.0f, 0.4f, 0.5f, 0.0f, 0.0f, 0.6f, 0.0f, 0.0f},
            0.58f, 0.35f, 0.55f, 0.15f, 0.35f, 0.45f, 0.65f, 0.25f, 0.45f,
            true, true
        };
        variants.push_back(v);
    }

    // Akan (Ghana) - Fontomfrom
    {
        RegionalVariant v;
        v.name = "Akan";
        v.region = "Ghana";
        v.profile = {
            "West African (Akan)",
            {1.0f, 0.0f, 0.0f, 0.85f, 0.0f, 0.5f, 0.9f, 0.0f,
             0.0f, 0.0f, 0.8f, 0.0f, 0.95f, 0.0f, 0.4f, 0.0f},
            {0.9f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f,
             0.85f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.4f, 0.0f},
            {0.5f, 0.0f, 0.7f, 0.4f, 0.6f, 0.0f, 0.8f, 0.3f,
             0.6f, 0.0f, 0.7f, 0.4f, 0.5f, 0.0f, 0.6f, 0.3f},
            {0.4f, 0.0f, 0.0f, 0.5f, 0.0f, 0.6f, 0.0f, 0.7f,
             0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.6f, 0.0f, 0.4f},
            0.60f, 0.3f, 0.5f, 0.12f, 0.32f, 0.45f, 0.7f, 0.2f, 0.4f,
            true, true
        };
        variants.push_back(v);
    }

    return variants;
}

// ========================================
// Afro-Cuban Variants
// ========================================
inline std::vector<RegionalVariant> getAfroCubanVariants() {
    std::vector<RegionalVariant> variants;

    // Son Clave (3-2)
    {
        RegionalVariant v;
        v.name = "Son (3-2)";
        v.region = "Cuba";
        v.profile = {
            "Afro-Cuban (Son 3-2)",
            {1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
             0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f},
            {0.9f, 0.0f, 0.0f, 0.0f, 0.7f, 0.0f, 0.0f, 0.0f,
             0.0f, 0.0f, 0.8f, 0.0f, 0.0f, 0.0f, 0.6f, 0.0f},
            {0.6f, 0.0f, 0.8f, 0.5f, 0.7f, 0.0f, 0.6f, 0.0f,
             0.5f, 0.0f, 0.7f, 0.4f, 0.6f, 0.0f, 0.8f, 0.0f},
            {0.5f, 0.0f, 0.0f, 0.4f, 0.0f, 0.6f, 0.0f, 0.7f,
             0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.6f, 0.0f, 0.0f},
            0.58f, 0.25f, 0.45f, 0.1f, 0.25f, 0.35f, 0.6f, 0.2f, 0.35f,
            true, true
        };
        variants.push_back(v);
    }

    // Rumba Clave (3-2)
    {
        RegionalVariant v;
        v.name = "Rumba (3-2)";
        v.region = "Cuba";
        v.profile = {
            "Afro-Cuban (Rumba 3-2)",
            {1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
             0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f},
            {0.9f, 0.0f, 0.0f, 0.0f, 0.7f, 0.0f, 0.0f, 0.0f,
             0.0f, 0.0f, 0.8f, 0.0f, 0.0f, 0.0f, 0.6f, 0.0f},
            {0.7f, 0.0f, 0.75f, 0.5f, 0.8f, 0.0f, 0.6f, 0.4f,
             0.5f, 0.0f, 0.7f, 0.5f, 0.65f, 0.0f, 0.75f, 0.0f},
            {0.6f, 0.0f, 0.0f, 0.5f, 0.0f, 0.7f, 0.0f, 0.75f,
             0.0f, 0.0f, 0.6f, 0.0f, 0.0f, 0.7f, 0.0f, 0.0f},
            0.60f, 0.3f, 0.5f, 0.12f, 0.28f, 0.4f, 0.65f, 0.25f, 0.4f,
            true, true
        };
        variants.push_back(v);
    }

    // Son Clave (2-3)
    {
        RegionalVariant v;
        v.name = "Son (2-3)";
        v.region = "Cuba";
        v.profile = {
            "Afro-Cuban (Son 2-3)",
            {0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
             1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f},
            {0.0f, 0.0f, 0.8f, 0.0f, 0.0f, 0.0f, 0.6f, 0.0f,
             0.9f, 0.0f, 0.0f, 0.0f, 0.7f, 0.0f, 0.0f, 0.0f},
            {0.5f, 0.0f, 0.7f, 0.5f, 0.6f, 0.0f, 0.8f, 0.0f,
             0.6f, 0.0f, 0.7f, 0.5f, 0.65f, 0.0f, 0.75f, 0.0f},
            {0.4f, 0.0f, 0.0f, 0.5f, 0.0f, 0.6f, 0.0f, 0.7f,
             0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.65f, 0.0f, 0.0f},
            0.58f, 0.25f, 0.45f, 0.1f, 0.25f, 0.35f, 0.6f, 0.2f, 0.35f,
            true, true
        };
        variants.push_back(v);
    }

    return variants;
}

// ========================================
// Brazilian Variants
// ========================================
inline std::vector<RegionalVariant> getBrazilianVariants() {
    std::vector<RegionalVariant> variants;

    // Rio Samba
    {
        RegionalVariant v;
        v.name = "Rio Samba";
        v.region = "Rio de Janeiro";
        v.profile = {
            "Brazilian (Rio Samba)",
            {0.8f, 0.0f, 0.5f, 0.0f, 0.9f, 0.0f, 0.5f, 0.0f,
             0.8f, 0.0f, 0.5f, 0.0f, 0.85f, 0.0f, 0.5f, 0.0f},
            {0.0f, 0.0f, 0.0f, 0.0f, 0.95f, 0.0f, 0.0f, 0.0f,
             0.0f, 0.0f, 0.0f, 0.0f, 0.9f, 0.0f, 0.0f, 0.0f},
            {0.8f, 0.6f, 0.7f, 0.5f, 0.9f, 0.6f, 0.7f, 0.5f,
             0.8f, 0.6f, 0.7f, 0.5f, 0.85f, 0.6f, 0.7f, 0.5f},
            {0.6f, 0.0f, 0.0f, 0.5f, 0.0f, 0.65f, 0.0f, 0.7f,
             0.0f, 0.0f, 0.55f, 0.0f, 0.0f, 0.6f, 0.0f, 0.0f},
            0.55f, 0.5f, 0.7f, 0.1f, 0.25f, 0.45f, 0.75f, 0.3f, 0.5f,
            true, true
        };
        variants.push_back(v);
    }

    // Bahia Samba-Reggae
    {
        RegionalVariant v;
        v.name = "Samba-Reggae";
        v.region = "Bahia";
        v.profile = {
            "Brazilian (Samba-Reggae)",
            {0.9f, 0.0f, 0.0f, 0.6f, 0.0f, 0.0f, 0.8f, 0.0f,
             0.0f, 0.6f, 0.0f, 0.0f, 0.85f, 0.0f, 0.0f, 0.5f},
            {0.95f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.7f, 0.0f,
             0.0f, 0.0f, 0.0f, 0.0f, 0.85f, 0.0f, 0.0f, 0.0f},
            {0.7f, 0.4f, 0.6f, 0.5f, 0.8f, 0.4f, 0.6f, 0.5f,
             0.7f, 0.4f, 0.65f, 0.5f, 0.75f, 0.4f, 0.6f, 0.5f},
            {0.5f, 0.0f, 0.0f, 0.4f, 0.0f, 0.55f, 0.0f, 0.6f,
             0.0f, 0.0f, 0.45f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f},
            0.52f, 0.4f, 0.6f, 0.1f, 0.25f, 0.4f, 0.65f, 0.25f, 0.45f,
            true, true
        };
        variants.push_back(v);
    }

    // Maracatu
    {
        RegionalVariant v;
        v.name = "Maracatu";
        v.region = "Pernambuco";
        v.profile = {
            "Brazilian (Maracatu)",
            {1.0f, 0.0f, 0.0f, 0.0f, 0.8f, 0.0f, 0.0f, 0.0f,
             0.9f, 0.0f, 0.0f, 0.0f, 0.85f, 0.0f, 0.0f, 0.0f},
            {0.95f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
             0.85f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
            {0.8f, 0.5f, 0.6f, 0.4f, 0.7f, 0.5f, 0.6f, 0.4f,
             0.75f, 0.5f, 0.6f, 0.4f, 0.7f, 0.5f, 0.6f, 0.4f},
            {0.6f, 0.0f, 0.0f, 0.5f, 0.0f, 0.6f, 0.0f, 0.65f,
             0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.55f, 0.0f, 0.0f},
            0.50f, 0.35f, 0.55f, 0.1f, 0.25f, 0.35f, 0.6f, 0.2f, 0.4f,
            false, true
        };
        variants.push_back(v);
    }

    return variants;
}

// ========================================
// Indian Variants
// ========================================
inline std::vector<RegionalVariant> getIndianVariants() {
    std::vector<RegionalVariant> variants;

    // Hindustani (North)
    {
        RegionalVariant v;
        v.name = "Hindustani";
        v.region = "North India";
        v.profile = {
            "Indian (Hindustani)",
            {1.0f, 0.0f, 0.0f, 0.0f, 0.7f, 0.0f, 0.0f, 0.0f,
             0.3f, 0.0f, 0.0f, 0.0f, 0.8f, 0.0f, 0.0f, 0.0f},
            {0.95f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
             0.4f, 0.0f, 0.0f, 0.0f, 0.7f, 0.0f, 0.0f, 0.0f},
            {0.6f, 0.4f, 0.5f, 0.3f, 0.65f, 0.4f, 0.5f, 0.35f,
             0.55f, 0.4f, 0.5f, 0.3f, 0.6f, 0.4f, 0.5f, 0.35f},
            {0.7f, 0.0f, 0.5f, 0.4f, 0.0f, 0.6f, 0.3f, 0.5f,
             0.0f, 0.4f, 0.6f, 0.0f, 0.5f, 0.4f, 0.0f, 0.3f},
            0.52f, 0.25f, 0.45f, 0.08f, 0.2f, 0.3f, 0.55f, 0.25f, 0.45f,
            false, false
        };
        variants.push_back(v);
    }

    // Carnatic (South)
    {
        RegionalVariant v;
        v.name = "Carnatic";
        v.region = "South India";
        v.profile = {
            "Indian (Carnatic)",
            {1.0f, 0.0f, 0.0f, 0.0f, 0.8f, 0.0f, 0.0f, 0.0f,
             0.6f, 0.0f, 0.0f, 0.0f, 0.85f, 0.0f, 0.0f, 0.0f},
            {0.95f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
             0.5f, 0.0f, 0.0f, 0.0f, 0.75f, 0.0f, 0.0f, 0.0f},
            {0.7f, 0.5f, 0.55f, 0.4f, 0.7f, 0.5f, 0.55f, 0.4f,
             0.65f, 0.5f, 0.55f, 0.4f, 0.7f, 0.5f, 0.55f, 0.4f},
            {0.75f, 0.0f, 0.55f, 0.45f, 0.0f, 0.65f, 0.35f, 0.55f,
             0.0f, 0.45f, 0.6f, 0.0f, 0.55f, 0.45f, 0.0f, 0.35f},
            0.50f, 0.3f, 0.5f, 0.1f, 0.22f, 0.35f, 0.6f, 0.3f, 0.5f,
            false, false
        };
        variants.push_back(v);
    }

    return variants;
}

// ========================================
// Gamelan Variants
// ========================================
inline std::vector<RegionalVariant> getGamelanVariants() {
    std::vector<RegionalVariant> variants;

    // Balinese
    {
        RegionalVariant v;
        v.name = "Balinese";
        v.region = "Bali";
        v.profile = {
            "Gamelan (Balinese)",
            {0.4f, 0.3f, 0.35f, 0.3f, 0.5f, 0.35f, 0.4f, 0.35f,
             0.6f, 0.4f, 0.5f, 0.4f, 0.8f, 0.5f, 0.6f, 1.0f},
            {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
             0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f},
            {0.8f, 0.0f, 0.75f, 0.0f, 0.85f, 0.0f, 0.8f, 0.0f,
             0.75f, 0.0f, 0.85f, 0.0f, 0.8f, 0.0f, 0.75f, 0.0f},
            {0.0f, 0.8f, 0.0f, 0.75f, 0.0f, 0.85f, 0.0f, 0.8f,
             0.0f, 0.75f, 0.0f, 0.85f, 0.0f, 0.8f, 0.0f, 0.75f},
            0.50f, 0.5f, 0.7f, 0.05f, 0.15f, 0.55f, 0.8f, 0.4f, 0.6f,
            false, true
        };
        variants.push_back(v);
    }

    // Javanese
    {
        RegionalVariant v;
        v.name = "Javanese";
        v.region = "Java";
        v.profile = {
            "Gamelan (Javanese)",
            {0.3f, 0.2f, 0.25f, 0.2f, 0.4f, 0.25f, 0.3f, 0.25f,
             0.5f, 0.3f, 0.4f, 0.3f, 0.7f, 0.4f, 0.5f, 1.0f},
            {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
             0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.95f},
            {0.7f, 0.0f, 0.65f, 0.0f, 0.75f, 0.0f, 0.7f, 0.0f,
             0.65f, 0.0f, 0.75f, 0.0f, 0.7f, 0.0f, 0.65f, 0.0f},
            {0.0f, 0.7f, 0.0f, 0.65f, 0.0f, 0.75f, 0.0f, 0.7f,
             0.0f, 0.65f, 0.0f, 0.75f, 0.0f, 0.7f, 0.0f, 0.65f},
            0.50f, 0.4f, 0.6f, 0.05f, 0.12f, 0.45f, 0.7f, 0.35f, 0.55f,
            false, true
        };
        variants.push_back(v);
    }

    return variants;
}

// ========================================
// Balkan Variants
// ========================================
inline std::vector<RegionalVariant> getBalkanVariants() {
    std::vector<RegionalVariant> variants;

    // Bulgarian 7/8
    {
        RegionalVariant v;
        v.name = "Rachenitsa";
        v.region = "Bulgaria";
        v.profile = {
            "Balkan (Bulgarian 7/8)",
            {1.0f, 0.4f, 0.8f, 0.4f, 0.85f, 0.5f, 0.6f, 0.0f,
             0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
            {0.95f, 0.0f, 0.7f, 0.0f, 0.8f, 0.0f, 0.0f, 0.0f,
             0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
            {0.8f, 0.5f, 0.7f, 0.5f, 0.75f, 0.6f, 0.5f, 0.0f,
             0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
            {0.6f, 0.0f, 0.5f, 0.0f, 0.55f, 0.4f, 0.0f, 0.0f,
             0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
            0.52f, 0.3f, 0.5f, 0.1f, 0.25f, 0.35f, 0.55f, 0.2f, 0.4f,
            false, false
        };
        variants.push_back(v);
    }

    // Turkish 9/8
    {
        RegionalVariant v;
        v.name = "Karsilama";
        v.region = "Turkey";
        v.profile = {
            "Balkan (Turkish 9/8)",
            {1.0f, 0.4f, 0.8f, 0.4f, 0.85f, 0.4f, 0.9f, 0.5f, 0.6f,
             0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
            {0.95f, 0.0f, 0.7f, 0.0f, 0.75f, 0.0f, 0.8f, 0.0f, 0.0f,
             0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
            {0.8f, 0.5f, 0.7f, 0.5f, 0.75f, 0.5f, 0.8f, 0.55f, 0.5f,
             0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
            {0.6f, 0.0f, 0.5f, 0.0f, 0.55f, 0.0f, 0.6f, 0.4f, 0.0f,
             0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
            0.52f, 0.3f, 0.5f, 0.1f, 0.25f, 0.35f, 0.55f, 0.2f, 0.4f,
            false, false
        };
        variants.push_back(v);
    }

    // Greek
    {
        RegionalVariant v;
        v.name = "Kalamatianos";
        v.region = "Greece";
        v.profile = {
            "Balkan (Greek 7/8)",
            {1.0f, 0.4f, 0.5f, 0.85f, 0.4f, 0.9f, 0.5f, 0.0f,
             0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
            {0.95f, 0.0f, 0.0f, 0.75f, 0.0f, 0.8f, 0.0f, 0.0f,
             0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
            {0.75f, 0.5f, 0.5f, 0.7f, 0.5f, 0.75f, 0.5f, 0.0f,
             0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
            {0.55f, 0.0f, 0.0f, 0.5f, 0.0f, 0.55f, 0.0f, 0.0f,
             0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
            0.52f, 0.3f, 0.5f, 0.1f, 0.25f, 0.35f, 0.55f, 0.2f, 0.4f,
            false, false
        };
        variants.push_back(v);
    }

    return variants;
}

// ========================================
// Regional Variant Manager
// ========================================
class RegionalVariantManager {
private:
    std::vector<std::vector<RegionalVariant>> allVariants;

public:
    RegionalVariantManager() {
        // Index matches style index in STYLES array
        allVariants.resize(10);
        allVariants[0] = getWestAfricanVariants();
        allVariants[1] = getAfroCubanVariants();
        allVariants[2] = getBrazilianVariants();
        allVariants[3] = getBalkanVariants();
        allVariants[4] = getIndianVariants();
        allVariants[5] = getGamelanVariants();
        // Jazz, Electronic, Breakbeat, Techno - no regional variants (modern styles)
    }

    int getNumVariants(int styleIndex) const {
        if (styleIndex >= 0 && styleIndex < static_cast<int>(allVariants.size())) {
            return static_cast<int>(allVariants[styleIndex].size());
        }
        return 0;
    }

    const RegionalVariant* getVariant(int styleIndex, int variantIndex) const {
        if (styleIndex >= 0 && styleIndex < static_cast<int>(allVariants.size())) {
            if (variantIndex >= 0 && variantIndex < static_cast<int>(allVariants[styleIndex].size())) {
                return &allVariants[styleIndex][variantIndex];
            }
        }
        return nullptr;
    }

    const StyleProfile* getVariantProfile(int styleIndex, int variantIndex) const {
        const RegionalVariant* v = getVariant(styleIndex, variantIndex);
        if (v) return &v->profile;
        return nullptr;
    }

    std::vector<std::string> getVariantNames(int styleIndex) const {
        std::vector<std::string> names;
        if (styleIndex >= 0 && styleIndex < static_cast<int>(allVariants.size())) {
            for (const auto& v : allVariants[styleIndex]) {
                names.push_back(v.name + " (" + v.region + ")");
            }
        }
        return names;
    }
};

} // namespace WorldRhythm
