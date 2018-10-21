/*
 * This file is part of Gen7TimeFinder
 * Copyright (C) 2018 by Admiral_Fish
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "IDSearcher.hpp"

IDSearcher::IDSearcher(QDateTime start, QDateTime end, u32 startFrame, u32 endFrame, Profile profile, IDFilter filter)
{
    startTime = std::move(start);
    endTime = std::move(end);
    this->startFrame = startFrame;
    this->endFrame = endFrame;
    this->profile = std::move(profile);
    this->filter = std::move(filter);
    cancel = false;
    progress = 0;
}

void IDSearcher::run()
{
    u64 epochStart = Utility::getCitraTime(startTime, profile.getOffset());
    u64 epochEnd = Utility::getCitraTime(endTime, profile.getOffset());

    for (u64 epoch = epochStart; epoch <= epochEnd; epoch += 1000)
    {
        u32 values[4] = { profile.getTick(), 0, static_cast<u32>(epoch & 0xffffffff), static_cast<u32>(epoch >> 32)};

        u32 initialSeed = Utility::calcInitialSeed(values);
        SFMT sfmt(initialSeed);
        sfmt.advanceFrames(startFrame);

        QVector<IDResult> results;
        for (u32 frame = startFrame; frame <= endFrame; frame++)
        {
            if (cancel)
                return;

            IDResult id(initialSeed, frame, sfmt.nextULong() & 0xffffffff);
            if (filter.compare(id))
            {
                QDateTime target = QDateTime::fromMSecsSinceEpoch(static_cast<qlonglong>(Utility::getNormalTime(epoch, profile.getOffset())), Qt::UTC);
                id.setTarget(target);
                results.append(id);
            }
        }

        if (!results.isEmpty())
            emit resultReady(results);
        emit updateProgress(++progress);
    }
}

int IDSearcher::maxProgress()
{
    auto val = static_cast<int>((Utility::getCitraTime(endTime, profile.getOffset()) - Utility::getCitraTime(startTime, profile.getOffset())) / 1000);
    return val;
}

void IDSearcher::cancelSearch()
{
    cancel = true;
}
