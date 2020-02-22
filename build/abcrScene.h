#pragma once

#include <Alembic\Abc\All.h>
#include <Alembic\AbcCoreOgawa\All.h>

#include "abcrGeom.h"

using namespace Alembic;

using namespace VVVV::PluginInterfaces::V1;
using namespace VVVV::PluginInterfaces::V2;

using namespace System;

namespace VVVV
{
namespace Nodes
{
namespace abcr
{
    class abcrScene
    {
        public:

            ~abcrScene();

            bool open(const string& path);

            void updateSample(float time);

            inline float getMaxTime() const { return m_maxTime; }

            bool getSample(const string& name);

        private:

            IArchive m_archive;
            std::unique_ptr<abcrGeom> m_top;

            chrono_t m_minTime;
            chrono_t m_maxTime;

            std::unordered_map<string, abcrGeom*> nameMap;
            std::unordered_map<string, abcrGeom*> fullnameMap;

    };
}
}
}