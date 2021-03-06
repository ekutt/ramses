//  -------------------------------------------------------------------------
//  Copyright (C) 2014 BMW Car IT GmbH
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#ifndef RAMSES_COMPONENTMOCKS_H
#define RAMSES_COMPONENTMOCKS_H

#include "framework_common_gmock_header.h"
#include "gmock/gmock.h"

#include "Resource/IResource.h"
#include "Scene/ClientScene.h"
#include "Scene/EScenePublicationMode.h"
#include "Components/ISceneGraphProviderComponent.h"
#include "Components/ManagedResource.h"
#include "Components/ISceneGraphConsumerComponent.h"
#include "Components/IResourceProviderComponent.h"
#include "Components/ResourceTableOfContents.h"
#include "Components/IResourceConsumerComponent.h"
#include "Components/ISceneProviderEventConsumer.h"
#include "SceneReferencing/SceneReferenceEvent.h"

namespace ramses_internal
{
    using namespace testing;

    class ResourceProviderComponentMock : public IResourceProviderComponent
    {
    public:
        ResourceProviderComponentMock();
        virtual ~ResourceProviderComponentMock() override;

        MOCK_METHOD(ManagedResource, manageResource, (const IResource& resource, bool deletionAllowed), (override));
        MOCK_METHOD(ManagedResourceVector, getResources, (), (override));
        MOCK_METHOD(ManagedResource, getResource, (ResourceContentHash hash), (override));
        MOCK_METHOD(ManagedResource, forceLoadResource, (const ResourceContentHash&), (override));
        MOCK_METHOD(ResourceHashUsage, getResourceHashUsage, (const ResourceContentHash&), (override));
        MOCK_METHOD(void, addResourceFile, (ResourceFileInputStreamSPtr resourceFileStream, const ResourceTableOfContents& toc), (override));
        MOCK_METHOD(bool, hasResourceFile, (const String&), (const, override));
        MOCK_METHOD(void, removeResourceFile, (const String& resourceFileName), (override));
        virtual void reserveResourceCount(uint32_t) override {};
    };

    class ResourceConsumerComponentMock : public IResourceConsumerComponent
    {
    public:
        ResourceConsumerComponentMock();
        virtual ~ResourceConsumerComponentMock() override;

        MOCK_METHOD(void, requestResourceAsynchronouslyFromFramework, (const ResourceContentHashVector& ids, const ResourceRequesterID& requesterID, const Guid& providerID), (override));
        MOCK_METHOD(void, cancelResourceRequest, (const ResourceContentHash& resourceHash, const ResourceRequesterID& requesterID), (override));
        MOCK_METHOD(ManagedResourceVector, popArrivedResources, (const ResourceRequesterID& requesterID), (override));
    };

    class SceneGraphProviderComponentMock : public ISceneGraphProviderComponent
    {
    public:
        SceneGraphProviderComponentMock();
        virtual ~SceneGraphProviderComponentMock() override;

        MOCK_METHOD(void, handleCreateScene, (ClientScene& scene, bool enableLocalOnlyOptimization, ISceneProviderEventConsumer& consumer), (override));
        MOCK_METHOD(void, handlePublishScene, (SceneId sceneId, EScenePublicationMode publicationMode), (override));
        MOCK_METHOD(void, handleUnpublishScene, (SceneId sceneId), (override));
        MOCK_METHOD(void, handleFlush, (SceneId sceneId, const FlushTimeInformation&, SceneVersionTag), (override));
        MOCK_METHOD(void, handleRemoveScene, (SceneId sceneId), (override));
    };

    class SceneGraphConsumerComponentMock : public ISceneGraphConsumerComponent
    {
    public:
        SceneGraphConsumerComponentMock();
        virtual ~SceneGraphConsumerComponentMock() override;

        MOCK_METHOD(void, subscribeScene, (const Guid& to, SceneId sceneId), (override));
        MOCK_METHOD(void, unsubscribeScene, (const Guid& to, SceneId sceneId), (override));
        MOCK_METHOD(void, sendSceneReferenceEvent, (const Guid& to, SceneReferenceEvent const& event), (override));

        virtual void setSceneRendererServiceHandler(ISceneRendererServiceHandler*) override
        {
        }
    };

    class SceneProviderEventConsumerMock : public ISceneProviderEventConsumer
    {
    public:
        SceneProviderEventConsumerMock();
        virtual ~SceneProviderEventConsumerMock() override;

        MOCK_METHOD(void, handleSceneReferenceEvent, (SceneReferenceEvent const& event, const Guid& rendererId), (override));
    };
}

#endif
